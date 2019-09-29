#include "DX12RenderBasic.h"

BEG_NAME_SPACE


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
