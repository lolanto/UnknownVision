#include "DX12RenderDevice.h"
#include <cassert>
#include <optional>

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
	/** 构建backbuffers */
	for (size_t index = 0; index < m_backBuffers.size(); ++index) {
		TextureInfo rtvInfo(ScreenWidth, ScreenHeight,
			m_backBuffers[index].Get(), D3D12_RESOURCE_STATE_COMMON | D3D12_RESOURCE_STATE_PRESENT);
		m_textures.insert(std::make_pair(TextureHandle(DEFAULT_BACK_BUFFER + index), rtvInfo));
	}

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
	/** 构建Descriptor heaps */
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;
	desc.NumDescriptors = NUMBER_OF_DESCRIPTOR_IN_RTV_HEAP;
	if (FAILED(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvHeap)))) {
		FLOG("%s: Create RTV heap failed\n", __FUNCTION__);
		return false;
	}
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	desc.NumDescriptors = NUMBER_OF_DESCRIPTOR_IN_DSV_HEAP;
	if (FAILED(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_dsvHeap)))) {
		FLOG("%s: Create RTV heap failed\n", __FUNCTION__);
		return false;
	}
	/** 构造默认viewport和裁剪矩形 */
	/** TODO: 提供用户定义viewport设置以及裁剪矩阵设置 */
	m_viewport.Width = ScreenWidth; m_viewport.Height = ScreenHeight;
	m_viewport.MaxDepth = 1.0f; m_viewport.MinDepth = 0.0f;
	m_viewport.TopLeftX = m_viewport.TopLeftY = 0;
	m_scissorRect.top = m_scissorRect.left = 0;
	m_scissorRect.right = ScreenWidth; m_scissorRect.bottom = ScreenHeight;
	/** 初始化必须的组件 */
	RenderDevice::Initialize(config);
	return true;
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

bool generateGraphicsRootSignature(const std::vector<RootSignatureParameter>& parameters, const std::vector<std::pair<SamplerDescriptor, RootSignatureParameter>>& staticSamplers) {
	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = parameters.size();
	rsDesc.NumStaticSamplers = staticSamplers.size();
	std::vector<D3D12_ROOT_PARAMETER> rsParas(parameters.size());
	for (size_t i = 0; i < parameters.size(); ++i) {
		switch (parameters[i].DecodeRegisterTypePrefix())
		{
		case RS_PARAMETER_TYPE_PREFIX_CONSTANT_BUFFER:
			uint32_t count = parameters[i].DecodeCountValue();
			if (count > 1) {
				/** 使用table */
				rsParas[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				rsParas[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}
			else {
				/** 使用descriptor */
			}
			break;
		case RS_PARAMETER_TYPE_PREFIX_CONSTANT_VALUE_32BIT:
			break;
		case RS_PARAMETER_TYPE_PREFIX_SAMPLER:
			break;
		case RS_PARAMETER_TYPE_PREFIX_TEXTURE:
			break;
		case RS_PARAMETER_TYPE_PREFIX_UNORDER_ACCESS:
			break;
		}
	}
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
		FLOG("%s: Create root signature failed!\n", __FUNCTION__);
		return false;
	}
	return true;
}

bool DX12RenderDevice::generateBuffer(const BufferDescriptor & desc)
{
	auto[ptr, state] = m_resourceManager.RequestBuffer(desc.size,
		ResourceStatusToResourceFlag(desc.status), ResourceStatusToHeapType(desc.status));
	if (ptr == nullptr) {
		FLOG("%s: Create buffer resource failed!\n", __FUNCTION__);
		return false;
	}
	auto[iter, res] = m_buffers.insert(std::make_pair(desc.handle, BufferInfo(desc.size, ptr, state)));
	if (res == false) {
		FLOG("%s: Insert new buffer failed!\n", __FUNCTION__);
		m_resourceManager.RevertBuffer(ptr, state);
		return false;
	}
	return true;
}

std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> DX12RenderDevice::PickupRenderTarget(TextureHandle tex) thread_safe {
	TextureInfo* texInfoPtr = PickupTexture(tex);
	if (texInfoPtr == nullptr) {
		FLOG("%s: Invalid texture handle value and can't pick up any render target!\n", __FUNCTION__);
		return {}; /**< 返回一个空的值 */
	}
	uint64_t& code = texInfoPtr->renderTargetViewCode;
	{
		std::lock_guard<decltype(m_rtvHeapGenLock)> lg(m_rtvHeapGenLock);
		if (TextureInfo::DecodeRTVCodeIndex(code) == NUMBER_OF_DESCRIPTOR_IN_RTV_HEAP ||
			m_rtvHeapGen[TextureInfo::DecodeRTVCodeIndex(code)] != TextureInfo::DecodeRTVCodeGen(code)) {
			/** 当前RTV不可用，需要重新创建 */
			uint32_t next = m_rtvHeapGen.back()++;
			if (next >= NUMBER_OF_DESCRIPTOR_IN_RTV_HEAP) {
				/** 当前RTV已满，需要回到开头位置 */
				m_rtvHeapGen.back() = 1;
				next = 0;
			}
			++m_rtvHeapGen[next];
			TextureInfo::EncodeRTVCodeIndex(code, next);
			TextureInfo::EncodeRTVCodeGen(code, m_rtvHeapGen[next]);

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
			D3D12_RESOURCE_DESC resDesc;
			resDesc = texInfoPtr->ptr->GetDesc();
			rtvDesc.Format = resDesc.Format;
			/** TODO: 目前只支持tex2d的0号Mipmap作为rendertarget */
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;
			m_device->CreateRenderTargetView(texInfoPtr->ptr, &rtvDesc,
				{ m_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr /**< 计算新的descriptor heap的位置 base + next * increment */
				+ next * m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] });
		}
	}
	return { { m_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr
		+ TextureInfo::DecodeRTVCodeIndex(code) * m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] } };
}

std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> DX12RenderDevice::PickupDepthStencilTarget(TextureHandle tex) thread_safe {
	TextureInfo* texInfoPtr = PickupTexture(tex);
	if (texInfoPtr == nullptr) {
		FLOG("%s: Invalid texture handle value and can't pick up any depth stencil!\n", __FUNCTION__);
		return {}; /**< 返回一个空的值 */
	}
	uint64_t& code = texInfoPtr->depthStencilViewCode;
	{
		std::lock_guard<decltype(m_dsvHeapGenLock)> lg(m_dsvHeapGenLock);
		if (TextureInfo::DecodeDSVCodeIndex(code) == NUMBER_OF_DESCRIPTOR_IN_DSV_HEAP ||
			m_dsvHeapGen[TextureInfo::DecodeDSVCodeIndex(code)] != TextureInfo::DecodeDSVCodeGen(code)) {
			/** 当前DSV不可用，需要重建 */
			uint32_t next = m_dsvHeapGen.back()++;
			if (next >= NUMBER_OF_DESCRIPTOR_IN_DSV_HEAP) {
				m_dsvHeapGen.back() = 1;
				next = 0;
			}
			++m_dsvHeapGen[next];
			TextureInfo::EncodeDSVCodeGen(code, m_dsvHeapGen[next]);
			TextureInfo::EncodeDSVCodeIndex(code, next);

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			D3D12_RESOURCE_DESC resDesc;
			resDesc = texInfoPtr->ptr->GetDesc();
			dsvDesc.Format = resDesc.Format;
			/** TODO: 目前仅支持DSV均可读写 */
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			/** TODO: 目前仅支持DSV为tex2d，且只有0号mipmap生效 */
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
			m_device->CreateDepthStencilView(texInfoPtr->ptr, &dsvDesc,
				{ m_dsvHeap->GetCPUDescriptorHandleForHeapStart().ptr
				+ next * m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] });
		}
	}
	return {
	{ m_dsvHeap->GetCPUDescriptorHandleForHeapStart().ptr
		+ TextureInfo::DecodeDSVCodeIndex(code) * m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]  }
	};
}


END_NAME_SPACE
