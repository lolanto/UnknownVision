#include "DX12Pipeline.h"
#include "DX12RenderDevice.h"

BEG_NAME_SPACE

extern DX12RenderDevice* GDX12RenderDevice;
extern DX12ShaderManager GDX12ShaderManager;

bool DX12PipelineManager::Build(DX12GraphicsPipelineObject& input, const DX12Shader* vs, const DX12Shader* ps)
{
	/** 分析shader object构造root signature */
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	std::vector<D3D12_ROOT_PARAMETER> parameters;
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
	/** 分析VS */
	auto&& packages = input.vs->Pack();
	for (auto& package : packages) {
		auto&& names = package->ShowParameterList();
		D3D12_ROOT_PARAMETER parameter;
		parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		size_t base = ranges.size();
		for (const auto& name : names) {
			auto range = vs->ranges.at(name);
			assert(range.RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER); /**< 从paceage中输出的name都是UAV, SRV或者CBV，不用考虑Sampler */
			ranges.push_back(range);
		}
		size_t numOfRanges = ranges.size() - base;
		if (numOfRanges) {
			parameter.DescriptorTable.NumDescriptorRanges = numOfRanges;
			parameter.DescriptorTable.pDescriptorRanges = reinterpret_cast<const D3D12_DESCRIPTOR_RANGE*>(base); /**< 将起始地址暂时存储于指针位置，之后再进行统一切换 */
			parameters.push_back(parameter);
		}
	}
	/** 目前足够构造VS的root signature，但是怎么建立parameter到descriptor heap之间的联系
	 * 最直接的想法是将对应关系另行存储，每个package对应一个descriptor heap的安排，每个参数名称对应descriptor heap index和内部的index */
	/** 万一出现两个shader使用了相同的参数名称呢？还是说在pipeline中也需要将参数按照shader进行区分 */

	/** 分析光栅方式，输入输出构造pso */
}

END_NAME_SPACE
