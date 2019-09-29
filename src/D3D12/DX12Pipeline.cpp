#include "DX12Pipeline.h"
#include "DX12RenderDevice.h"

BEG_NAME_SPACE

extern DX12RenderDevice* GDX12RenderDevice;
extern DX12ShaderManager GDX12ShaderManager;

void DX12GraphicsPipelineObject::Reset() {
	m_pso.Reset(); m_rs.Reset();
	m_NameToDescriptorSettings.clear();
	m_DescriptorTableSettings.clear();
	m_cachedViews.clear();
}

bool DX12PipelineManager::Build(DX12GraphicsPipelineObject& pipelineObject, const DX12Shader* vs, const DX12Shader* ps)
{
	pipelineObject.Reset();
	/** 分析shader object构造root signature */
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	std::vector<D3D12_ROOT_PARAMETER> parameters;
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;

	/** 分析shader object的Descriptor heap的构造 */
	std::vector<DescriptorTableSetting> descriptorTableSettings;
	std::map<std::string, DescriptorSetting> nameToDescriptorSettings;

	auto shaderAnalyseHelper = [&parameters, &ranges, &descriptorTableSettings, &nameToDescriptorSettings, &staticSamplers]
	(const BasicShader* pBasicShader,  const DX12Shader* pDXShader, D3D12_SHADER_VISIBILITY visibility)
	{
		auto&& packages = pBasicShader->Pack();
		for (auto& package : packages) {
			auto&& names = package->ShowParameterList();
			D3D12_ROOT_PARAMETER parameter;
			DescriptorTableSetting table;
			table.cachedHandle.ptr = UINT64_MAX; /**< 初始化，无缓存 */
			table.type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			table.numOfDescriptors = 0;
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameter.ShaderVisibility = visibility;
			size_t rangeBase = ranges.size();
			uint16_t descriptorOffset = 0;
			uint16_t tableIndex = descriptorTableSettings.size();
			table.offsetFromBegin = rangeBase;
			for (const auto& name : names) {
				auto range = pDXShader->ranges.at(name);
				assert(range.RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER); /**< 从paceage中输出的name都是UAV, SRV或者CBV，不用考虑Sampler */
				ranges.push_back(range);
				DescriptorSetting descriptorSetting;
				descriptorSetting.descriptorOffset = descriptorOffset++;
				descriptorSetting.tableIndex = tableIndex;
				nameToDescriptorSettings.insert(std::make_pair(name, descriptorSetting));
			}
			size_t numOfRanges = ranges.size() - rangeBase;
			if (numOfRanges) {
				parameter.DescriptorTable.NumDescriptorRanges = numOfRanges;
				parameter.DescriptorTable.pDescriptorRanges = reinterpret_cast<const D3D12_DESCRIPTOR_RANGE*>(rangeBase); /**< 将起始地址暂时存储于指针位置，之后再进行统一切换 */
				parameters.push_back(parameter);
				table.numOfDescriptors = numOfRanges;
				descriptorTableSettings.push_back(table);
			}
		}
		auto&& samplers = pBasicShader->GetSamplerNameAndPointers();
		for (auto& sampler : samplers) {
			auto range = pDXShader->ranges.at(sampler.first);
			D3D12_STATIC_SAMPLER_DESC desc = AnalyseStaticSamplerFromSamplerDescriptor(*sampler.second, range.RegisterSpace, range.BaseShaderRegister);
			staticSamplers.emplace_back(desc);
		}
	};
	/** 分析VS */
	shaderAnalyseHelper(pipelineObject.vs, vs, D3D12_SHADER_VISIBILITY_VERTEX);
	/** 分析PS */
	shaderAnalyseHelper(pipelineObject.ps, ps, D3D12_SHADER_VISIBILITY_PIXEL);
	/** TODO: 万一出现两个shader使用了相同的参数名称呢？还是说在pipeline中也需要将参数按照shader进行区分 */
	/** 修改parameter的指针，构造root signature desc */
	rootSignatureDesc.NumParameters = parameters.size();
	for (auto& parameter : parameters) {
		size_t rangeBase = reinterpret_cast<size_t>(parameter.DescriptorTable.pDescriptorRanges);
		parameter.DescriptorTable.pDescriptorRanges = &ranges[rangeBase];
	}
	rootSignatureDesc.pParameters = parameters.data();
	rootSignatureDesc.NumStaticSamplers = staticSamplers.size();
	rootSignatureDesc.pStaticSamplers = staticSamplers.data();
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	/** RootSignature构造完成，编译生成对象 */
	{
		SmartPTR<ID3DBlob> rootSignatureBlob;
		SmartPTR<ID3DBlob> errorBlob;
		if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1,
			rootSignatureBlob.GetAddressOf(), errorBlob.GetAddressOf()))) {
			assert(false); /**< Root Signature 编译失败 */
		}
		assert(SUCCEEDED(m_pDevice->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
			rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&pipelineObject.m_rs))));
	}
	/** root signature成功创建，修改Pipeline的descriptor setting */
	pipelineObject.m_NameToDescriptorSettings.swap(nameToDescriptorSettings);
	pipelineObject.m_DescriptorTableSettings.swap(descriptorTableSettings);
	/** 分析光栅方式，输入输出构造pso */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPSODesc;
	graphicsPSODesc.NodeMask = 0;
	/** Output Stage */
	{
		auto outputStageOptions = pipelineObject.outputOpt();
		graphicsPSODesc.BlendState = AnalyseBlendingOptionsFromOutputStageOptions(outputStageOptions);
		graphicsPSODesc.DepthStencilState = AnalyseDepthStencilOptionsFromOutputStageOptions(outputStageOptions);
		graphicsPSODesc.DSVFormat = ElementFormatToDXGIFormat(outputStageOptions.dsvFormat);
		UINT numOfRTV = 0;
		for (size_t i = 0; i < ArraySize(graphicsPSODesc.RTVFormats); ++i) {
			if (i >= MAX_RENDER_TARGET) graphicsPSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
			else {
				graphicsPSODesc.RTVFormats[i] = ElementFormatToDXGIFormat(outputStageOptions.rtvFormats[i]);
				if (outputStageOptions.rtvFormats[i] != ELEMENT_FORMAT_TYPE_INVALID) ++numOfRTV;
			}
		}
		graphicsPSODesc.NumRenderTargets = numOfRTV;
		/** TODO: blend state 也不考虑使用Sample Mask */
		graphicsPSODesc.SampleMask = 0xffffffff;
	}
	/** Raterization Stage */
	{
		auto rasterizationStageOptions = pipelineObject.rastOpt();
		graphicsPSODesc.RasterizerState = AnalyseRasterizerStatesFromRasterizeOptions(rasterizationStageOptions);
		graphicsPSODesc.PrimitiveTopologyType = PrimitiveTypeToPrimitiveTopologyType(rasterizationStageOptions.primitive);
		/** TODO: 暂时不考虑使用多重采样 */
		graphicsPSODesc.SampleDesc.Count = 1;
		graphicsPSODesc.SampleDesc.Quality = 0;
	}
	/** input layout */
	auto inputLayout = pipelineObject.vtxAttribList();
	graphicsPSODesc.InputLayout.NumElements = inputLayout.size();
	std::vector<D3D12_INPUT_ELEMENT_DESC> layouts(inputLayout.size());
	for (size_t i = 0; i < inputLayout.size(); ++i)
		layouts[i] = AnalyseInputElementDescFromVertexAttribute(inputLayout[i]);
	graphicsPSODesc.InputLayout.pInputElementDescs = layouts.data();
	/** rs */
	graphicsPSODesc.pRootSignature = pipelineObject.m_rs.Get();
	/** shaders */
	graphicsPSODesc.VS.BytecodeLength = vs->shaderByteCode->GetBufferSize();
	graphicsPSODesc.VS.pShaderBytecode = vs->shaderByteCode->GetBufferPointer();
	graphicsPSODesc.PS.BytecodeLength = ps->shaderByteCode->GetBufferSize();
	graphicsPSODesc.PS.pShaderBytecode = ps->shaderByteCode->GetBufferPointer();
	/** TODO: Others */
	{
		graphicsPSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
		graphicsPSODesc.CachedPSO.pCachedBlob = nullptr;
		/** 只有使用triangle strip时候有效 */
		graphicsPSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		/** output stream暂时不适用 */
		graphicsPSODesc.StreamOutput.NumEntries = 0;
		graphicsPSODesc.StreamOutput.NumStrides = 0;
		graphicsPSODesc.StreamOutput.pBufferStrides = nullptr;
		graphicsPSODesc.StreamOutput.pSODeclaration = nullptr;
		graphicsPSODesc.StreamOutput.RasterizedStream = 0;
		graphicsPSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}
	assert(SUCCEEDED(m_pDevice->CreateGraphicsPipelineState(
		&graphicsPSODesc, IID_PPV_ARGS(&pipelineObject.m_pso))));

	return true;
}

END_NAME_SPACE
