#include "DX12RenderBasic.h"

BEG_NAME_SPACE



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

D3D12_SAMPLER_DESC AnalyseSamplerFromSamplerDescriptor(const SamplerDescriptor& desc) thread_safe {
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

D3D12_SAMPLER_DESC AnalyseSamplerFromSamplerSettings(FilterType filter,
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
