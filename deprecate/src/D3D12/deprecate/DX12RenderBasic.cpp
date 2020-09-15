#include "DX12RenderBasic.h"

BEG_NAME_SPACE



D3D12_BLEND_DESC AnalyseBlendingOptionsFromOutputStageOptions(const OutputStageOptions & osOpt) thread_safe
{
	/** TODO: 完善对blend的支持，目前仅提供默认(无blend)操作 */
	D3D12_BLEND_DESC desc;
	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		desc.RenderTarget[i] = defaultRenderTargetBlendDesc;
	return desc;
}

D3D12_DEPTH_STENCIL_DESC AnalyseDepthStencilOptionsFromOutputStageOptions(const OutputStageOptions & osOpt) thread_safe
{
	/** TODO: 完善深度模板操作的支持，目前仅支持默认的深度测试，不支持模板测试 */
	D3D12_DEPTH_STENCIL_DESC desc;
	desc.DepthEnable = osOpt.enableDepthTest;
	desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	desc.StencilEnable = FALSE;
desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
desc.FrontFace = defaultStencilOp;
desc.BackFace = defaultStencilOp;
return desc;
}

D3D12_RASTERIZER_DESC AnalyseRasterizerOptionsFromRasterizeOptions(const RasterizeOptions & rastOpt) thread_safe
{
	D3D12_RASTERIZER_DESC desc;
	desc.FillMode = FillModeToDX12FillMode(rastOpt.fillMode);
	desc.CullMode = CullModeToCullMode(rastOpt.cullMode);
	desc.FrontCounterClockwise = rastOpt.counterClockWiseIsFront;
	/** TODO: 支持以下光栅化设置 */
	desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	desc.DepthClipEnable = TRUE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	desc.ForcedSampleCount = 0;
	desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return desc;
}

D3D12_STATIC_SAMPLER_DESC AnalyseStaticSamplerFromSamplerDescriptor(const SamplerDescriptor & desc, uint8_t spaceIndex, uint8_t registerIndex) thread_safe
{
	D3D12_STATIC_SAMPLER_DESC samplerDesc;
	samplerDesc.RegisterSpace = spaceIndex;
	samplerDesc.ShaderRegister = registerIndex;
	samplerDesc.Filter = FilterTypeToDX12FilterType(desc.filter);
	samplerDesc.AddressU = SamplerAddressModeToDX12TextureAddressMode(desc.uAddrMode);
	samplerDesc.AddressV = SamplerAddressModeToDX12TextureAddressMode(desc.vAddrMode);
	samplerDesc.AddressW = SamplerAddressModeToDX12TextureAddressMode(desc.wAddrMode);
	/** 静态sampler不能提供可设置的边界颜色只能使用默认值黑/白 */
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	/** TODO: 以下设置暂时不支持，均采用默认操作 */
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	return samplerDesc;
}

D3D12_SAMPLER_DESC AnalyseSamplerFromSamperDescriptor(const SamplerDescriptor& desc) thread_safe {
	D3D12_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = FilterTypeToDX12FilterType(desc.filter);
	samplerDesc.AddressU = SamplerAddressModeToDX12TextureAddressMode(desc.uAddrMode);
	samplerDesc.AddressV = SamplerAddressModeToDX12TextureAddressMode(desc.vAddrMode);
	samplerDesc.AddressW = SamplerAddressModeToDX12TextureAddressMode(desc.wAddrMode);
	samplerDesc.BorderColor[0] = desc.borderColor[0];
	samplerDesc.BorderColor[1] = desc.borderColor[1];
	samplerDesc.BorderColor[2] = desc.borderColor[2];
	samplerDesc.BorderColor[3] = desc.borderColor[3];
	/** TODO: 以下设置暂时不支持，均采用默认操作 */
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	return samplerDesc;
}

D3D12_SAMPLER_DESC AnalyseSamplerFromSamperSettings(FilterType filter,
	const SamplerAddressMode(&uvwMode)[3],
	const float(&borderColor)[4]) thread_safe {
	D3D12_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = FilterTypeToDX12FilterType(filter);
	samplerDesc.AddressU = SamplerAddressModeToDX12TextureAddressMode(uvwMode[0]);
	samplerDesc.AddressV = SamplerAddressModeToDX12TextureAddressMode(uvwMode[1]);
	samplerDesc.AddressW = SamplerAddressModeToDX12TextureAddressMode(uvwMode[2]);
	samplerDesc.BorderColor[0] = borderColor[0];
	samplerDesc.BorderColor[1] = borderColor[1];
	samplerDesc.BorderColor[2] = borderColor[2];
	samplerDesc.BorderColor[3] = borderColor[3];
	/** TODO: 以下设置暂时不支持，均采用默认操作 */
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	return samplerDesc;
}

auto RootSignatureInfo::QueryWithParameter(const std::vector<RootSignatureParameter>& qury)
-> std::vector<std::optional<RootSignatureQueryAnswer>>
{
	std::vector<std::optional<RootSignatureQueryAnswer> > anses(qury.size());
	auto iter = anses.begin();
	for (const auto& q : qury) {
		RootSignatureQueryAnswer ans;
		for (size_t idx = 0; idx < parameters.size(); ++idx) {
			if (parameters[idx].DecodeParameterType() == q.DecodeParameterType()) {
				if (auto r = parameters[idx].DecodeToDescriptorRange()) {
					if (r.value().RegisterSpace == q.DecodeSpaceValue()
						&& r.value().BaseShaderRegister <= q.DecodeBaseRegister()
						&& r.value().BaseShaderRegister + r.value().NumDescriptors >= q.DecodeBaseRegister() + q.DecodeCountValue()) {
						/** 解析询问结果 */
					}
				}
				else if (auto d = parameters[idx].DecodeToSingleRootParameter()) {

				}
			}
		}
	}
}

END_NAME_SPACE
