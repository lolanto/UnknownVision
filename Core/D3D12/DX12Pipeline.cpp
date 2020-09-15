#include "DX12Pipeline.h"
#include "DX12RenderDevice.h"
#include "DX12Shader.h"
#include "../../Utility/InfoLog/InfoLog.h"
BEG_NAME_SPACE

extern DX12ShaderManager GDX12ShaderManager;

void DX12GraphicsPipelineObject::Reset() {
	m_pso.Reset(); m_rs.Reset();
}

DX12GraphicsPipelineObject* DX12PipelineManager::Build(DX12GraphicsPipelineObject& pipelineObject, const DX12Shader* vs, const DX12Shader* ps)
{
	pipelineObject.Reset();
	/** 分析shader object构造root signature */
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	std::vector<D3D12_ROOT_PARAMETER> rootSignatureParameters;
	std::list<std::vector<D3D12_DESCRIPTOR_RANGE>> rangesList;
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
	auto shaderAnalyseHelper = [&rootSignatureParameters, &rangesList, &staticSamplers]
	(const BasicShader* pBasicShader,  const DX12Shader* pDXShader, D3D12_SHADER_VISIBILITY visibility)
	{
		auto&& parameterGroups = pBasicShader->GetShaderParameters();
		if (parameterGroups.empty()) return;
		for (const auto& parameters : parameterGroups) {
			D3D12_ROOT_PARAMETER parameter;
			parameter.ShaderVisibility = visibility;
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			size_t rangesCount = 0;
			rangesList.push_back({});
			for (const auto& e : parameters) {
				if (e.paramType == SHADER_PARAMETER_TYPE_SAMPLER) {
					auto&& desc = AnalyseStaticSamplerFromSamplerDescriptor(e.samplerDesc(), e.space, e.slot);
					desc.ShaderVisibility = visibility;
					staticSamplers.push_back(std::move(desc));
				}
				else {
					D3D12_DESCRIPTOR_RANGE range;
					range.BaseShaderRegister = e.slot;
					range.NumDescriptors = e.count;
					range.RegisterSpace = e.space;
					range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
					switch (e.paramType) {
					case SHADER_PARAMETER_TYPE_BUFFER_R:
						range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						break;
					case SHADER_PARAMETER_TYPE_TEXTURE_R:
						range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						break;
					case SHADER_PARAMETER_TYPE_BUFFER_RW:
					case SHADER_PARAMETER_TYPE_TEXTURE_RW:
						range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
						break;
					}
					rangesList.back().push_back(range);
					++rangesCount;
				}
			}
			if (rangesCount > 0) {
				parameter.DescriptorTable.NumDescriptorRanges = rangesCount;
				parameter.DescriptorTable.pDescriptorRanges = rangesList.back().data();
				rootSignatureParameters.push_back(parameter);
			}
		}
	};
	/** 分析VS */
	shaderAnalyseHelper(pipelineObject.vs, vs, D3D12_SHADER_VISIBILITY_VERTEX);
	/** 分析PS */
	shaderAnalyseHelper(pipelineObject.ps, ps, D3D12_SHADER_VISIBILITY_PIXEL);

	/** 修改parameter的指针，构造root signature desc */
	rootSignatureDesc.NumParameters = rootSignatureParameters.size();
	rootSignatureDesc.pParameters = rootSignatureParameters.data();
	rootSignatureDesc.NumStaticSamplers = staticSamplers.size();
	rootSignatureDesc.pStaticSamplers = staticSamplers.data();
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	/** RootSignature构造完成，编译生成对象 */
	{
		SmartPTR<ID3DBlob> rootSignatureBlob;
		SmartPTR<ID3DBlob> errorBlob;
		if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			rootSignatureBlob.GetAddressOf(), errorBlob.GetAddressOf()))) {
			LOG_ERROR("Compile Root Signature failed!");
			abort();
		}
		if (FAILED(m_pDevice->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
			rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&pipelineObject.m_rs)))) {
			LOG_ERROR("Create Root Signature failed!");
			abort();
		}
	}
	/** root signature成功创建，修改Pipeline的descriptor setting */
	/** 分析光栅方式，输入输出构造pso */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPSODesc;
	memset(&graphicsPSODesc, 0, sizeof(graphicsPSODesc));
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
	if (FAILED(m_pDevice->CreateGraphicsPipelineState(
		&graphicsPSODesc, IID_PPV_ARGS(&pipelineObject.m_pso)))) {
		LOG_ERROR("Create Graphics pipeline state failed!");
		abort();
	}
	m_graphicsPSOs.push_back(std::move(pipelineObject));
	return &m_graphicsPSOs.back();
}

END_NAME_SPACE
