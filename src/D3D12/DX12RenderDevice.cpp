#include "DX12RenderBasic.h"
#include <cassert>

#define XifFailed(function, behavior) if (FAILED(function)) behavior

BEG_NAME_SPACE

bool DX12RenderDevice::Initialize(std::string config)
{
	/** 构建特殊资源 */
	for (uint8_t idx = 0; idx < NUMBER_OF_BACK_BUFFERS; ++idx) {
		XifFailed(m_swapChain->GetBuffer(idx, IID_PPV_ARGS(&m_backBuffers[idx])), {
			FLOG("%s: Create Back buffer %d FAILED!\n", __FUNCTION__, idx);
			return false;
		});
	}
	m_textures.insert(std::make_pair(TextureHandle(DEFAULT_BACK_BUFFER), TextureInfo(m_backBuffers[0].Get(), 
		D3D12_RESOURCE_STATE_RENDER_TARGET, ScreenWidth, ScreenHeight)));
#ifdef _DEBUG
	/** 检查所有的特殊资源都已经构建完成 */
	for (uint8_t i = 0; i < NUMBER_OF_SPECIAL_BUFFER_RESOURCE; ++i) {
		auto res = m_buffers.find(BufferHandle(i));
		assert(res != m_buffers.end() && res->second.ptr != nullptr);
	}
	for (uint8_t i = 0; i < NUMBER_OF_SPECIAL_TEXTURE_RESOURCE; ++i) {
		auto res = m_textures.find(TextureHandle(i));
		assert(res != m_textures.end() && res->second.ptr != nullptr);
	}
#endif // _DEBUG

	/** 初始化必须的组件 */
	RenderDevice::Initialize(config);
	return true;
}

void DX12RenderDevice::Process()
{
	if (m_state != DEVICE_STATE_RUNNING) return;

	Task curTask;
	{
		std::lock_guard<OptimisticLock> lg(m_taskQueueLock);
		if (m_taskQueue.empty()) return;
		curTask = std::move(m_taskQueue.front());
		m_taskQueue.pop();
	}
	/** 按顺序处理Task中的指令，同时根据指令类型执行相应的处理函数 */
	for (const auto& cmd : curTask.Commands) {
		switch (cmd.type) {
		case Command::COMMAND_TYPE_TEST:
			TEST_func(cmd);
			break;
		default:
			FLOG("%s: command %d is not found!\n", __FUNCTION__, cmd.type);
		}
	}
}

void DX12RenderDevice::TEST_func(const Command & cmd)
{
#ifdef _DEBUG
	assert(cmd.type == Command::COMMAND_TYPE_TEST);
#endif // _DEBUG
	auto& descriptor = cmd.parameters[0].buf;
	if (descriptor.handle == BufferHandle::InvalidIndex()) return;
	if (m_buffers.find(descriptor.handle) != m_buffers.end()) return;
	BufferInfo bufInfo(descriptor.size);
	{
		auto[resPtr, resState] = m_resourceManager.RequestBuffer(descriptor.size, 
			ResourceStatusToResourceFlag(descriptor.status),
			ResourceStatusToHeapType(descriptor.status));
		assert(resPtr != nullptr);
		bufInfo.ptr = resPtr;
		bufInfo.state = resState;
	}
	{
		auto[iter, isInserted] = m_buffers.insert(std::make_pair(descriptor.handle, bufInfo));
		if (isInserted == false) MLOG("Insert Buffer Failed!\n");
	}
	return;
}


bool DX12RenderDevice::generateGraphicsPSO(const ProgramDescriptor & pmgDesc)
{
	{
		auto iter = m_psoAndRootSig.find(pmgDesc.handle);
		if (iter != m_psoAndRootSig.end() && iter->second.pso != nullptr) {
			return true; /**< 已经生成过pso就不重复创建 */
		}
	}
	PSOAndRootSig par;
	if (generateGraphicsRootSignature(pmgDesc, par.rootSignature) == false) {
		FLOG("%s: Generate Root signature failed!\n", __FUNCTION__);
		return false;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; /**< 对结构体进行零初始化 */
	const DX12RenderBackend::ProgramInfo& pmgRef = m_backend.AccessProgramInfo(pmgDesc.handle);
	/** BlendState设置 */
	psoDesc.BlendState = AnalyseBlendingOptionsFromOutputStageOptions(pmgDesc.osOpt);
	psoDesc.SampleMask = UINT_MAX; /**< blend过程使用的mask，暂不清楚具体用途 */
	/** 暂时不清楚这个cache有何用 */
	psoDesc.CachedPSO = {};
	/** DepthStencil设置 */
	psoDesc.DepthStencilState = AnalyseDepthStencilOptionsFromOutputStageOptions(pmgDesc.osOpt);
	/** 设置深度模板缓冲的像素格式 */
	psoDesc.DSVFormat = ElementFormatToDXGIFormat(pmgDesc.osOpt.dsvFormat);
	/** 非WRAP类型的设备默认设置为NONE */
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	/** 图元类型为triangle strip时候设置有效，用于让一个顶点缓冲能够包含多个strip，以stripCutValue为索引的值代表strip中断位置
	 * TODO: 考虑是否支援此类操作 */
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	/** 用于多GPU情况 */
	psoDesc.NodeMask = 0;
	/** 设置图元类型以及光栅化过程的设置 */
	psoDesc.PrimitiveTopologyType = PrimitiveTypeToPrimitiveTopologyType(pmgDesc.rastOpt.primitive);
	psoDesc.RasterizerState = AnalyseRasterizerOptionsFromRasterizeOptions(pmgDesc.rastOpt);
	/** TODO: 暂不支持multisample */
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	/** TODO: 暂不支持streamoutput */
	psoDesc.StreamOutput = {};
	/** 设置inputLayout */
	psoDesc.InputLayout.pInputElementDescs = pmgRef.inputLayoutPtr->data();
	psoDesc.InputLayout.NumElements = pmgRef.inputLayoutPtr->size();
	/** 设置RTV格式 */
	{
		uint32_t numRTV = 0;
		for (; numRTV < 8
			&& pmgDesc.osOpt.rtvFormats[numRTV] != ELEMENT_FORMAT_TYPE_INVALID;
			++numRTV)
			psoDesc.RTVFormats[numRTV] = ElementFormatToDXGIFormat(pmgDesc.osOpt.rtvFormats[numRTV]);
		psoDesc.NumRenderTargets = numRTV;
	}
	/** 设置rootsignature */
	psoDesc.pRootSignature = par.rootSignature.Get();
	/** 设置shaders
	 * requestProgram时候已经确保了必须有vs和ps */
	for (const auto& shaderName : pmgDesc.shaders.names) {
		if (shaderName.size() == 0) continue;
		const auto& shaderInfo = m_backend.AccessShaderInfo(shaderName);
		switch (shaderInfo.Type()) {
		case SHADER_TYPE_VERTEX_SHADER:
			psoDesc.VS.BytecodeLength = shaderInfo.blob->GetBufferSize();
			psoDesc.VS.pShaderBytecode = shaderInfo.blob->GetBufferPointer();
			break;
		case SHADER_TYPE_PIXEL_SHADER:
			psoDesc.PS.BytecodeLength = shaderInfo.blob->GetBufferSize();
			psoDesc.PS.pShaderBytecode = shaderInfo.blob->GetBufferPointer();
			break;
		case SHADER_TYPE_GEOMETRY_SHADER:
			psoDesc.GS.BytecodeLength = shaderInfo.blob->GetBufferSize();
			psoDesc.GS.pShaderBytecode = shaderInfo.blob->GetBufferPointer();
			break;
		case SHADER_TYPE_HULL_SHADER:
			psoDesc.HS.BytecodeLength = shaderInfo.blob->GetBufferSize();
			psoDesc.HS.pShaderBytecode = shaderInfo.blob->GetBufferPointer();
			break;
		case SHADER_TYPE_TESSELLATION_SHADER:
			psoDesc.DS.BytecodeLength = shaderInfo.blob->GetBufferSize();
			psoDesc.DS.pShaderBytecode = shaderInfo.blob->GetBufferPointer();
			break;
		default:
			FLOG("%s: Invalid shader type. Its name is %s", __FUNCTION__, shaderName.c_str());
			return false;
		}
	}
	if (FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&par.pso)))) {
		FLOG("%s: Create Graphics pipeline state object failed!\n", __FUNCTION__);
		return false;
	}
	auto res = m_psoAndRootSig.insert(std::make_pair(pmgDesc.handle, par));
	if (res.second == false) {
		FLOG("%s: Generate new Pipelinestate object and root signature failed! Can't Insert them into the map\n", __FUNCTION__);
		return false;
	}
	return true;
}

bool DX12RenderDevice::generateGraphicsRootSignature(const ProgramDescriptor & pmgDesc, SmartPTR<ID3D12RootSignature>& rootSignature)
{
	const DX12RenderBackend::ProgramInfo& pmgRef = m_backend.AccessProgramInfo(pmgDesc.handle);
	std::vector<D3D12_DESCRIPTOR_RANGE> scuRanges; /**< 记录srv, cbv以及uav的descriptor range */
	/** 处理SRV, CBV, UAV资源描述 */
	if (pmgRef.srv_cbv_uav_Desc.size() != 0) {
		std::vector<uint32_t> srv_cbv_uav_idx;
		for (const auto& idx : pmgRef.srv_cbv_uav_Desc) {
			uint32_t resHeapIndex = DX12RenderBackend::ProgramInfo::DecodeIndex(idx.second);
			if (resHeapIndex >= srv_cbv_uav_idx.size())
				srv_cbv_uav_idx.resize(resHeapIndex + 1);
			srv_cbv_uav_idx[resHeapIndex] = idx.second;
		}
		D3D12_DESCRIPTOR_RANGE curRange;
		/** 初始化第一个range */
		curRange.RangeType = DX12RenderBackend::ProgramInfo::DecodeType(srv_cbv_uav_idx[0]);
		curRange.NumDescriptors = 1;
		curRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		curRange.RegisterSpace = DX12RenderBackend::ProgramInfo::DecodeSpaceIndex(srv_cbv_uav_idx[0]);
		curRange.BaseShaderRegister = DX12RenderBackend::ProgramInfo::DecodeRegisterIndex(srv_cbv_uav_idx[0]);
		for (int i = 1; i < srv_cbv_uav_idx.size(); ++i) {
			if (curRange.RangeType == DX12RenderBackend::ProgramInfo::DecodeType(srv_cbv_uav_idx[i])
				&& curRange.RegisterSpace == DX12RenderBackend::ProgramInfo::DecodeSpaceIndex(srv_cbv_uav_idx[i])
				&& curRange.BaseShaderRegister + curRange.NumDescriptors == DX12RenderBackend::ProgramInfo::DecodeRegisterIndex(srv_cbv_uav_idx[i])) {
				++curRange.NumDescriptors;
			}
			else {
				/** 需要创建新的range了 */
				scuRanges.push_back(curRange);
				curRange.RangeType = DX12RenderBackend::ProgramInfo::DecodeType(srv_cbv_uav_idx[i]);
				curRange.NumDescriptors = 1;
				curRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				curRange.RegisterSpace = DX12RenderBackend::ProgramInfo::DecodeSpaceIndex(srv_cbv_uav_idx[i]);
				curRange.BaseShaderRegister = DX12RenderBackend::ProgramInfo::DecodeRegisterIndex(srv_cbv_uav_idx[i]);
			}
		}
		scuRanges.push_back(curRange);
	}
	/** 处理sampler描述 */
	std::vector<D3D12_DESCRIPTOR_RANGE> samplerRanges;
	if (pmgRef.sampler_Desc.size() != 0) {
		std::vector<uint32_t> sampler_idx;
		for (const auto& idx : pmgRef.sampler_Desc) {
			uint32_t samplerHeapIndex = DX12RenderBackend::ProgramInfo::DecodeIndex(idx.second);
			if (samplerHeapIndex >= sampler_idx.size())
				sampler_idx.resize(samplerHeapIndex + 1);
			sampler_idx[samplerHeapIndex] = idx.second;
		}
		D3D12_DESCRIPTOR_RANGE curRange;
		curRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		curRange.NumDescriptors = 1;
		curRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		curRange.RegisterSpace = DX12RenderBackend::ProgramInfo::DecodeSpaceIndex(sampler_idx[0]);
		curRange.BaseShaderRegister = DX12RenderBackend::ProgramInfo::DecodeRegisterIndex(sampler_idx[0]);
		for (int i = 1; i < sampler_idx.size(); ++i) {
			if (curRange.RegisterSpace == DX12RenderBackend::ProgramInfo::DecodeSpaceIndex(sampler_idx[i])
				&& curRange.BaseShaderRegister + curRange.NumDescriptors == DX12RenderBackend::ProgramInfo::DecodeRegisterIndex(sampler_idx[i])) {
				++curRange.NumDescriptors;
			}
			else {
				samplerRanges.push_back(curRange);
				curRange.BaseShaderRegister = DX12RenderBackend::ProgramInfo::DecodeRegisterIndex(sampler_idx[i]);
				curRange.RegisterSpace = DX12RenderBackend::ProgramInfo::DecodeSpaceIndex(sampler_idx[i]);
				curRange.NumDescriptors = 1;
			}
		}
		samplerRanges.push_back(curRange);
	}

	CD3DX12_ROOT_PARAMETER parameters[2];
	parameters[0].InitAsDescriptorTable(scuRanges.size(), scuRanges.data());
	parameters[1].InitAsDescriptorTable(samplerRanges.size(), samplerRanges.data());
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSig;
	rootSig.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
	rootSig.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSig.Desc_1_0.pStaticSamplers = nullptr;
	rootSig.Desc_1_0.NumStaticSamplers = 0;
	rootSig.Desc_1_0.NumParameters = 2;
	rootSig.Desc_1_0.pParameters = (D3D12_ROOT_PARAMETER*)&parameters;
	SmartPTR<ID3DBlob> serializeRootSignature;
#ifdef _DEBUG
	SmartPTR<ID3DBlob> errorMsg;
	if (FAILED(D3D12SerializeVersionedRootSignature(&rootSig, serializeRootSignature.GetAddressOf(), errorMsg.GetAddressOf()))) {
		LFLOG(256, "%s: Serialize root signature failed, \n  ErrMsg is: %s \n", __FUNCTION__, (char*)errorMsg->GetBufferPointer());
		/** TODO: 暂不清楚error blob中的数据为什么 */
		return false;
	}
#else // _DEBUG
	if (FAILED(D3D12SerializeVersionedRootSignature(&rootSig, serializeRootSignature.GetAddressOf(), nullptr))) {
		return false;
	}
#endif // _DEBUG
	/** TODO: 暂不支持多GPU */
	if (FAILED(m_device->CreateRootSignature(0, serializeRootSignature->GetBufferPointer(), serializeRootSignature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)))) {
		FLOG( "%s: Create root signature failed!\n", __FUNCTION__);
		return false;
	}
	return true;
}

END_NAME_SPACE
