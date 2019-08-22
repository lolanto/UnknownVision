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
			for (uint8_t target = 0; target < getSV_TARGET(shader.PS_IO); ++target) {
				signature.insert(std::make_pair(RENDER_TARGET_NAME[target], Parameter::PARAMETER_TYPE_TEXTURE));
			}
			for (uint8_t depth = 0; depth < getSV_DEPTH(shader.PS_IO); ++depth) {
				signature.insert(std::make_pair(DEPTH_TARGET_NAME[depth], Parameter::PARAMETER_TYPE_TEXTURE));
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
	if (usedIndex) signature.insert(std::make_pair(INDEX_BUFFER_NAME, Parameter::PARAMETER_TYPE_BUFFER));
	ProgramHandle newProgramHandle(m_nextProgramHandle++);

	{
		std::lock_guard<decltype(m_programLock)> lg(m_programLock);
		m_programs.insert(std::make_pair(newProgramHandle, newProgramInfo));
	}

	return 	ProgramDescriptor(std::move(signature), shaderNames, newProgramHandle,
		newProgramType, usedIndex, rasterization, outputStage);
}

ProgramHandle DX12RenderDevice::RequestProgram2(const ShaderNames& shaderNames, VertexAttributeHandle va_handle,
	bool usedIndex, RasterizeOptions rasterization, OutputStageOptions outputStage,
	const std::map<std::string, const SamplerDescriptor&>& staticSamplers) {
	/** 构建pos以及root signature */

	return ProgramHandle::InvalidIndex();
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

TextureHandle DX12RenderDevice::RequestTexture(uint32_t width, uint32_t height, ElementFormatType type,
	ResourceStatus status) thread_safe {
	TextureInfo info(width, height);
	//if (status.usage == RESOURCE_USAGE_SHADER_RESOURCE) {
	//	info.shaderResViewDesc.Format = ElementFormatToDXGIFormat(type);
	//	/** TODO: 暂时只支持一个2维纹理 */
	//	info.shaderResViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//	/** 该参数的解析详见: https://www.gamedev.net/forums/topic/693749-shader4componentmapping-explain/ */
	//	info.shaderResViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//	/** TODO:  尚未支持mipmap */
	//	info.shaderResViewDesc.Texture2D.MipLevels = -1;
	//	info.shaderResViewDesc.Texture2D.MostDetailedMip = 0;
	//	info.shaderResViewDesc.Texture2D.PlaneSlice = 0;
	//	info.shaderResViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	//}
	//if (status.usage == RESOURCE_USAGE_RENDER_TARGET) {
	//	/** TODO: 暂时只支持一个2维纹理的渲染输出，且输出在0级mipmap上 */
	//	info.renderTargetViewDesc.Format = ElementFormatToDXGIFormat(type);
	//	info.renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	//	info.renderTargetViewDesc.Texture2D.MipSlice = 0;
	//	info.renderTargetViewDesc.Texture2D.PlaneSlice = 0;
	//}
	//if (status.usage == RESOURCE_USAGE_DEPTH_STENCIL) {
	//	/** TODO: 暂时只支持一个2维纹理的深度模板缓存，且两部分均是可写的，都作用于0级mipmap上 */
	//	info.depthStencilViewDesc.Format = ElementFormatToDXGIFormat(type);
	//	info.depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//	info.depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE; /**< Depth 以及 Stencil部分都是可被管线写的 */
	//	info.depthStencilViewDesc.Texture2D.MipSlice = 0;
	//}
	auto[ptr, state] = m_resourceManager.RequestTexture(width, height, ElementFormatToDXGIFormat(type),
		ResourceStatusToResourceFlag(status), 0);
	if (ptr == nullptr) {
		FLOG("%s: Create Texture resource failed!\n", __FUNCTION__);
		return TextureHandle::InvalidIndex();
	}
	info.ptr = ptr; info.state = state;
	TextureHandle&& handle = RenderDevice::RequestTexture(width, height, type, status);
	std::lock_guard<OptimisticLock> lg(m_textureLock);
	auto[iter, res] = m_textures.insert(std::make_pair(handle, info));
	assert(res);
	return handle;
}

bool DX12RenderDevice::RevertResource(TextureHandle handle) thread_safe {
	std::lock_guard<decltype(m_textureLock)> lg(m_textureLock);
	auto iter = m_textures.find(handle);
	if (iter == m_textures.end()) {
		FLOG("%s: Invalid texture handle and revert resource failed!\n", __FUNCTION__);
		return false;
	}
	if (m_resourceManager.RevertTexture(iter->second.ptr, iter->second.state) == false) {
		FLOG("%s: Revert texture failed!\n", __FUNCTION__);
		return false;
	}
	m_textures.erase(iter);
	return true;
}

BufferHandle DX12RenderDevice::RequestBuffer(size_t size, ResourceStatus status, size_t stride) thread_safe {
	if (status.usage == RESOURCE_USAGE_INDEX_BUFFER && (
		stride != 8 && stride != 16 && stride != 32
		)) {
		FLOG("%s: Invalid index type: %zu, valid index type is 8, 16 or 32\n", __FUNCTION__, stride);
		return BufferHandle::InvalidIndex();
	}
	auto[ptr, state] = m_resourceManager.RequestBuffer(size,
		ResourceStatusToResourceFlag(status), ResourceStatusToHeapType(status));
	if (ptr == nullptr) {
		FLOG("%s: Create buffer resource failed!\n", __FUNCTION__);
		return BufferHandle::InvalidIndex();
	}
	BufferInfo info(size, ptr, state);
	if (status.usage == RESOURCE_USAGE_CONSTANT_BUFFER) {
		info.constBufViewDesc.BufferLocation = ptr->GetGPUVirtualAddress();
		info.constBufViewDesc.SizeInBytes = size;
	}
	else if (status.usage == RESOURCE_USAGE_VERTEX_BUFFER) {
		info.vtxBufView.BufferLocation = ptr->GetGPUVirtualAddress();
		info.vtxBufView.SizeInBytes = size;
		info.vtxBufView.StrideInBytes = stride;
	}
	else if (status.usage == RESOURCE_USAGE_INDEX_BUFFER) {
		info.idxBufView.BufferLocation = ptr->GetGPUVirtualAddress();
		info.idxBufView.SizeInBytes = size;
		switch (stride) {
		case 8:
			info.idxBufView.Format = DXGI_FORMAT_R8_UINT;
			break;
		case 16:
			info.idxBufView.Format = DXGI_FORMAT_R16_UINT;
			break;
		case 32:
			info.idxBufView.Format = DXGI_FORMAT_R32_UINT;
			break;
		}
	}
	else {
		FLOG("%s: Invalid buffer usage\n", __FUNCTION__);
		assert(false);
	}
	BufferHandle&& handle = RenderDevice::RequestBuffer(size, status);
	std::lock_guard<OptimisticLock> lg(m_bufferLock);
	auto[iter, res] = m_buffers.insert(std::make_pair(handle, info));
	assert(res);
	return handle;
}

BufferInfo* DX12RenderDevice::PickupBuffer(BufferHandle handle) thread_safe {
	std::lock_guard<decltype(m_bufferLock)> lg(m_bufferLock);
	auto iter = m_buffers.find(handle);
	if (iter == m_buffers.end()) {
		FLOG("%s: Can't find out any buffer associate with this handle\n", __FUNCTION__);
		return nullptr;
	}
	return &iter->second;
}

TextureInfo* DX12RenderDevice::PickupTexture(TextureHandle handle) thread_safe {
	std::lock_guard<decltype(m_textureLock)> lg(m_textureLock);
	auto iter = m_textures.find(handle);
	if (iter == m_textures.end()) {
		FLOG("%s: Can't find out any texture associate with this handle\n", __FUNCTION__);
		return nullptr;
	}
	return &iter->second;
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

//bool DX12RenderDevice::ExecuteProgram(ProgramHandle program, const std::vector<BufferHandle>& vtxbufs,
//	const std::vector<TextureHandle>& renderTargets,
//	const std::vector<TextureHandle>& depthStencils,
//	const std::vector<std::pair<std::string, Parameter>>& parameterList,
//	const BufferHandle idxbuf) {
	//const ProgramInfo* pmgPtr = nullptr;
	//{
	//	std::lock_guard<OptimisticLock> lg(m_programLock);
	//	auto iter = m_programs.find(program);
	//	if (iter == m_programs.end()) {
	//		FLOG("%s: Invalid program handle\n", __FUNCTION__);
	//		return false;
	//	}
	//	pmgPtr = &iter->second;
	//}
	///** 构造descriptor heap */
	//D3D12_CPU_DESCRIPTOR_HANDLE srv_cbv_uav_cpuHandle(m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart());
	//D3D12_CPU_DESCRIPTOR_HANDLE sampler_cpuHandle(m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]->GetCPUDescriptorHandleForHeapStart());
	//for (const auto& para : parameterList) {
	//	auto iter = pmgPtr->resNameToEncodingValue.find(para.first);
	//	if (iter == pmgPtr->resNameToEncodingValue.end()) {
	//		FLOG("%s: No such parameter: %s\n", __FUNCTION__, para.first.c_str());
	//		continue;
	//	}
	//	uint32_t index = ProgramInfo::DecodeIndex(iter->second);
	//	if (para.second.type == Parameter::PARAMETER_TYPE_BUFFER) {
	//		std::lock_guard<decltype(m_bufferLock)> lg(m_bufferLock);
	//		auto iter = m_buffers.find(para.second.buf.handle);
	//		assert(iter != m_buffers.end());
	//		m_device->CreateConstantBufferView(&iter->second.constBufViewDesc, { srv_cbv_uav_cpuHandle.ptr + 
	//			m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] * index });
	//	}
	//	else if (para.second.type == Parameter::PARAMETER_TYPE_TEXTURE) {
	//		std::lock_guard<decltype(m_textureLock)> lg(m_textureLock);
	//		auto iter = m_textures.find(para.second.tex.handle);
	//		assert(iter != m_textures.end());
	//		m_device->CreateShaderResourceView(iter->second.ptr, &iter->second.shaderResViewDesc, { srv_cbv_uav_cpuHandle.ptr +
	//			m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] * index });
	//	}
	//	else if (para.second.type == Parameter::PARAMETER_TYPE_SAMPLER) {
	//		std::lock_guard<decltype(m_samplerLock)> lg(m_samplerLock);
	//		auto iter = m_samplers.find(para.second.sampler.handle);
	//		assert(iter != m_samplers.end());
	//		m_device->CreateSampler(&iter->second.desc, { sampler_cpuHandle.ptr +
	//			m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] * index });
	//	}
	//	else {
	//		FLOG("%s: Invalid parameter type for %s\n", __FUNCTION__, para.first.c_str());
	//	}
	//}
	//
	///** 构造vtx buffer views */
	//std::vector<D3D12_VERTEX_BUFFER_VIEW> vtxBufViews(vtxbufs.size());
	//{
	//	std::lock_guard<decltype(m_bufferLock)> lg(m_bufferLock);
	//	for (size_t i = 0; i < vtxbufs.size(); ++i)
	//		vtxBufViews[i] = m_buffers[vtxbufs[i]].vtxBufView;
	//}

	///** 构造render target view descriptor heap */
	//D3D12_CPU_DESCRIPTOR_HANDLE rtv_cpuHandle(m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart());
	//{
	//	std::lock_guard<decltype(m_textureLock)> lg(m_textureLock);
	//	for (const auto rt : renderTargets) {
	//		auto iter = m_textures.find(rt);
	//		assert(iter != m_textures.end());
	//		m_device->CreateRenderTargetView(iter->second.ptr, &iter->second.renderTargetViewDesc, rtv_cpuHandle);
	//		rtv_cpuHandle.ptr += m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];
	//	}
	//}

	///** 构造depth stencil view descriptor heap */
	//D3D12_CPU_DESCRIPTOR_HANDLE dsv_cpuHandle(m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart());
	//{
	//	std::lock_guard<decltype(m_textureLock)> lg(m_textureLock);
	//	for (const auto ds : depthStencils) {
	//		auto iter = m_textures.find(ds);
	//		assert(iter != m_textures.end());
	//		m_device->CreateDepthStencilView(iter->second.ptr, &iter->second.depthStencilViewDesc, dsv_cpuHandle);
	//		dsv_cpuHandle.ptr += m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_DSV];
	//	}
	//}

	///** 设置管线 */
	//m_cmdList[0]->SetPipelineState(pmgPtr->pso.Get());
	//m_cmdList[0]->SetGraphicsRootSignature(pmgPtr->rootSignature.Get());

	///** 绑定数据到管线上 */
	///** 绑定IA阶段 */
	//m_cmdList[0]->IASetVertexBuffers(0, vtxBufViews.size(), vtxBufViews.data());
	//UINT numberOfInstance = 0;
	//if (idxbuf != BufferHandle::InvalidIndex()) {
	//	std::lock_guard<decltype(m_bufferLock)> lg(m_bufferLock);
	//	auto iter = m_buffers.find(idxbuf);
	//	assert(iter != m_buffers.end());
	//	switch (iter->second.idxBufView.Format) {
	//	case DXGI_FORMAT_R8_UINT:
	//		numberOfInstance = iter->second.idxBufView.SizeInBytes / 8;
	//		break;
	//	case DXGI_FORMAT_R16_UINT:
	//		numberOfInstance = iter->second.idxBufView.SizeInBytes / 16;
	//		break;
	//	case DXGI_FORMAT_R32_UINT:
	//		numberOfInstance = iter->second.idxBufView.SizeInBytes / 32;
	//		break;
	//	default:
	//		FLOG("%s: Invalid index type\n", __FUNCTION__);
	//		return false;
	//	}
	//	m_cmdList[0]->IASetIndexBuffer(&iter->second.idxBufView);
	//}
	///** 绑定OM阶段 */
	//m_cmdList[0]->OMSetRenderTargets(renderTargets.size(), &dsv_cpuHandle, true, &dsv_cpuHandle);
	//ID3D12DescriptorHeap* heaps[2] = { m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Get(),
	//	m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].Get() };
	//m_cmdList[0]->SetDescriptorHeaps(2, heaps);
	//m_cmdList[0]->SetGraphicsRootDescriptorTable(0, m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart());
	//m_cmdList[0]->SetGraphicsRootDescriptorTable(1, m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]->GetGPUDescriptorHandleForHeapStart());
	///** TODO: 暂不支持多视口 */
	//m_cmdList[0]->RSSetViewports(1, &m_viewport);
	//m_cmdList[0]->RSSetScissorRects(1, &m_scissorRect);
	///** TODO: 暂不支持自定义图元类型 */
	//m_cmdList[0]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//m_cmdList[0]->DrawIndexedInstanced(3, numberOfInstance, 0, 0, 0);
	//m_cmdList[0]->Close();

//	return true;
//}

END_NAME_SPACE
