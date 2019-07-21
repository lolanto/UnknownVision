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

ProgramDescriptor DX12RenderDevice::RequestProgram(const ShaderNames& shaderNames, VertexAttributeHandle va_handle,
	bool usedIndex, RasterizeOptions rasterization, OutputStageOptions outputStage,
	const std::map<std::string, const SamplerDescriptor&>& staticSamplers) thread_safe {
	/** 先校验shader组合的合法性 */
	ProgramType newProgramType;
	if (shaderNames[SHADER_TYPE_VERTEX_SHADER].size() != 0 &&
		shaderNames[SHADER_TYPE_PIXEL_SHADER].size() != 0)
		newProgramType = PROGRAM_TYPE_GRAPHICS;
	else if (shaderNames[SHADER_TYPE_COMPUTE_SHADER].size() != 0)
		newProgramType = PROGRAM_TYPE_COMPUTE;
	else {
		FLOG("%s: Invalid shader group!\n", __FUNCTION__);
		return ProgramDescriptor::CreateInvalidDescirptor();
	}

	std::map<std::string, Parameter::Type> signature; /**< 存储该program所有签名 */
	ProgramInfo newProgramInfo;
	/** 一些临时数据，用于先行收集shader的srv, cbv, uav信息，供之后的排序使用 */
	std::list<uint32_t> srv_cbv_uav_idx;
	std::map<std::string, std::list<uint32_t>::iterator> name_srv_cbv_uav_ptr;
	std::list<uint32_t> sampler_idx;
	std::map<std::string, std::list<uint32_t>::iterator> name_sampler_ptr;
	/** 利用插入排序，将编码插入到指定的列表中
	 * @param list 需要插入的列表
	 * @param value 需要插入的编码值
	 * @return 返回被插入的值在列表中的迭代器*/
	auto insertCodeIntoList = [](std::list<uint32_t>& list, uint32_t value)->std::list<uint32_t>::iterator {
		auto iter = list.begin();
		while (iter != list.end() && value > (*iter)) ++iter;
		if (iter == list.end()) {
			list.push_back(value);
			return std::prev(list.end());
		}
		else {
			return list.insert(iter, value);
		}
	};
	/** 创建static sampler 并存储到programInfo中，创建成功返回true，创建失败返回false */
	auto generateStaticSamplerAndStoreIntoProgramInfo = [&newProgramInfo, &staticSamplers](
		const std::string& samplerName, const DX12RenderBackend::ShaderInfo::ResourceInfo& info) -> bool {
		auto samplerIter = staticSamplers.find(samplerName); 
		if (samplerIter == staticSamplers.end()) return false; /**< 该sampler并不需要设置为static */
		newProgramInfo.staticSamplers.push_back(
			AnalyseStaticSamplerFromSamplerDescriptor(
				samplerIter->second, info.spaceIndex, info.registerIndex));
		return true;
	};
	/** 逐个shader分析 */
	for (uint8_t i = 0; i < SHADER_TYPE_NUMBER_OF_TYPE &&
		shaderNames[i].size() != 0; ++i) {
		ShaderType curType = static_cast<ShaderType>(i);
		const DX12RenderBackend::ShaderInfo& shader = m_backend.UpdateShaderInfo(shaderNames[i].c_str(), curType);
		/** 分析资源签名 */
		for (const auto& vSig : shader.signatures) {
			uint32_t idx = 0;
			ProgramInfo::EncodeReigsterIndex(idx, vSig.second.registerIndex);
			ProgramInfo::EncodeSpaceIndex(idx, vSig.second.spaceIndex);
			switch (vSig.second.type) {
				/** TODO: 暂时还没有支持UAV */
			case D3D_SIT_CBUFFER:
				signature.insert(std::make_pair(vSig.first, Parameter::PARAMETER_TYPE_BUFFER));
				ProgramInfo::EncodeType(idx, ProgramInfo::RESOURCE_TYPE_CONSTANT_BUFFER_VIEW);
				name_srv_cbv_uav_ptr.insert(std::make_pair(vSig.first, insertCodeIntoList(srv_cbv_uav_idx, idx)));
				break;
			case D3D_SIT_TEXTURE:
				signature.insert(std::make_pair(vSig.first, Parameter::PARAMETER_TYPE_TEXTURE));
				ProgramInfo::EncodeType(idx, ProgramInfo::RESOURCE_TYPE_SHADER_RESOURCE_VIEW);
				name_srv_cbv_uav_ptr.insert(std::make_pair(vSig.first, insertCodeIntoList(srv_cbv_uav_idx, idx)));
				break;
			case D3D_SIT_SAMPLER:
				if (generateStaticSamplerAndStoreIntoProgramInfo(vSig.first, vSig.second)) break; /**< 当前sampler是static sampler不加入到签名中 */
				signature.insert(std::make_pair(vSig.first, Parameter::PARAMETER_TYPE_SAMPLER));
				ProgramInfo::EncodeType(idx, ProgramInfo::RESOURCE_TYPE_SAMPLER_STATE);
				name_sampler_ptr.insert(std::make_pair(vSig.first, insertCodeIntoList(sampler_idx, idx)));
				break;
			default:
				FLOG("%s: Doesn't support parameter: %s\n", __FUNCTION__, vSig.first.c_str());
			}
		}
		/** 校验IA的输入是否匹配 */
		if (shader.Type() == SHADER_TYPE_VERTEX_SHADER) {
			const auto& ILSE = m_backend.AccessVertexAttributeDescs(va_handle);
			if (VS_IO_ExceedMatch(ILSE.VS_IO, shader.VS_IO) == false) {
				FLOG("%s: IA Stage setting doesn't match this program\n", __FUNCTION__);
				return ProgramDescriptor::CreateInvalidDescirptor();
			}
			newProgramInfo.inputLayoutPtr = &ILSE.layout;
			/** 将IA的签名合并到整个签名中 */
			signature.insert(ILSE.inputLayoutSignature.begin(), ILSE.inputLayoutSignature.end());
		}
		/** 设置输出接口 */
		else if (shader.Type() == SHADER_TYPE_PIXEL_SHADER) {
			std::string semantic = "COLOR";
			for (uint8_t target = 0; target < getSV_TARGET(shader.PS_IO); ++target) {
				signature.insert(std::make_pair(semantic + std::to_string(target), Parameter::PARAMETER_TYPE_TEXTURE));
			}
			semantic = "DEPTH";
			for (uint8_t depth = 0; depth < getSV_DEPTH(shader.PS_IO); ++depth) {
				signature.insert(std::make_pair(semantic + std::to_string(depth), Parameter::PARAMETER_TYPE_TEXTURE));
			}
		}
	}
	{
		/** 构建Descriptor列表 */
		std::list<uint32_t>::iterator iter;
		iter = srv_cbv_uav_idx.begin();
		for (int i = 0; i < srv_cbv_uav_idx.size(); ++i, ++iter) {
			(*iter) += i;
		}
		for (const auto& e : name_srv_cbv_uav_ptr) {
			newProgramInfo.resNameToEncodingValue.insert(std::make_pair(e.first, *e.second));
		}
		iter = sampler_idx.begin();
		for (int i = 0; i < sampler_idx.size(); ++i, ++iter) {
			(*iter) += i;
		}
		for (const auto& e : name_sampler_ptr) {
			newProgramInfo.resNameToEncodingValue.insert(std::make_pair(e.first, *e.second));
		}
	}
	/** 假如使用索引，需要添加索引签名 */
	if (usedIndex) signature.insert(std::make_pair("IDXBUF", Parameter::PARAMETER_TYPE_BUFFER));
	ProgramHandle newProgramHandle(m_nextProgramHandle++);

	{
		std::lock_guard<decltype(m_programLock)> lg(m_programLock);
		m_programs.insert(std::make_pair(newProgramHandle, newProgramInfo));
	}

	return 	ProgramDescriptor(std::move(signature), shaderNames, newProgramHandle,
		newProgramType, usedIndex, rasterization, outputStage);
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

	auto& programInfo = m_programs.find(pmgDesc.handle)->second;
	if (programInfo.pso != nullptr) {
		return true; /**< 已经生成过pso就不重复创建 */
	}

	if (generateGraphicsRootSignature(programInfo, programInfo.rootSignature) == false) {
		FLOG("%s: Generate Root signature failed!\n", __FUNCTION__);
		return false;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; /**< 对结构体进行零初始化 */
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
	psoDesc.InputLayout.pInputElementDescs = programInfo.inputLayoutPtr->data();
	psoDesc.InputLayout.NumElements = programInfo.inputLayoutPtr->size();
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
	psoDesc.pRootSignature = programInfo.rootSignature.Get();
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
	if (FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&programInfo.pso)))) {
		FLOG("%s: Create Graphics pipeline state object failed!\n", __FUNCTION__);
		return false;
	}
	return true;
}

bool DX12RenderDevice::generateGraphicsRootSignature(const ProgramInfo & pmgRef, SmartPTR<ID3D12RootSignature>& rootSignature)
{
	std::vector<D3D12_DESCRIPTOR_RANGE> scuRanges; /**< 记录srv, cbv以及uav的descriptor range */
	std::vector<D3D12_DESCRIPTOR_RANGE> samplerRanges; /**< 记录sampler的descriptor range */
	auto generateDescriptorRangeFromEncodingValue = []( std::vector<D3D12_DESCRIPTOR_RANGE>& container,
		const std::vector<uint32_t> encodingValues ) -> void {
		if (encodingValues.size() != 0) {
			D3D12_DESCRIPTOR_RANGE newRange;
			newRange.RangeType = ProgramInfo::DecodeTypeToDescriptorRangeType(encodingValues[0]);
			newRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			newRange.NumDescriptors = 1;
			newRange.RegisterSpace = ProgramInfo::DecodeSpaceIndex(encodingValues[0]);
			newRange.BaseShaderRegister = ProgramInfo::DecodeRegisterIndex(encodingValues[0]);
			for (int i = 1; i < encodingValues.size(); ++i) {
				if (newRange.RangeType == ProgramInfo::DecodeTypeToDescriptorRangeType(encodingValues[i])
					&& newRange.RegisterSpace == ProgramInfo::DecodeSpaceIndex(encodingValues[i])
					&& newRange.BaseShaderRegister + newRange.NumDescriptors == ProgramInfo::DecodeRegisterIndex(encodingValues[i])) {
					++newRange.NumDescriptors;
				}
				else {
					/** 需要创建新的range了 */
					container.push_back(newRange);
					newRange.RangeType = ProgramInfo::DecodeTypeToDescriptorRangeType(encodingValues[i]);
					newRange.NumDescriptors = 1;
					newRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
					newRange.RegisterSpace = ProgramInfo::DecodeSpaceIndex(encodingValues[i]);
					newRange.BaseShaderRegister = ProgramInfo::DecodeRegisterIndex(encodingValues[i]);
				}
			}
			container.push_back(newRange);
		}
	};
	if (pmgRef.resNameToEncodingValue.size() != 0) {
		std::vector<uint32_t> samplerEncodingValues;
		std::vector<uint32_t> scuEncodingValues;
		/** 将原来存储于programInfo中的string -> heapIndexEncode信息，根据heapIndex重新进行一次排序
		 * 因为需要产生描述连续范围的descriptor range，而原来记录的map中不能直接提取heapIndex的顺序信息
		 * 只能逐个分析并提取排序 */
		for (const auto& idx : pmgRef.resNameToEncodingValue) {
			uint32_t resHeapIndex = ProgramInfo::DecodeIndex(idx.second);
			D3D12_DESCRIPTOR_RANGE_TYPE type = ProgramInfo::DecodeTypeToDescriptorRangeType(idx.second);
			std::vector<uint32_t>& encodingValues = type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER ? samplerEncodingValues : scuEncodingValues;
			if (resHeapIndex >= encodingValues.size()) {
				encodingValues.resize(resHeapIndex + 1);
			}
			encodingValues[resHeapIndex] = idx.second;
		}
		generateDescriptorRangeFromEncodingValue(scuRanges, scuEncodingValues);
		generateDescriptorRangeFromEncodingValue(samplerRanges, samplerEncodingValues);
	}

	CD3DX12_ROOT_PARAMETER parameters[2];
	parameters[0].InitAsDescriptorTable(scuRanges.size(), scuRanges.data());
	parameters[1].InitAsDescriptorTable(samplerRanges.size(), samplerRanges.data());
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSig;
	rootSig.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
	rootSig.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSig.Desc_1_0.NumStaticSamplers = pmgRef.staticSamplers.size();
	rootSig.Desc_1_0.pStaticSamplers = pmgRef.staticSamplers.size() == 0 ? nullptr : pmgRef.staticSamplers.data();
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
