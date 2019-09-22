#include "DX12Shader.h"

BEG_NAME_SPACE

ShaderHandle DX12ShaderManager::Compile(const wchar_t* srcFilePath, ShaderType type)
{
	/** TODO: 避免相同Shader多次编译 */
	DXCompilerHelper dxc;
	std::string profile;
	switch (type) {
	case SHADER_TYPE_VERTEX_SHADER: profile = "vs_6_0"; break;
	case SHADER_TYPE_PIXEL_SHADER: profile = "ps_6_0"; break;
	default:
		assert(false);
	}
	DX12Shader newShader;
	assert(dxc.CompileToByteCode(srcFilePath, profile.c_str(), newShader.shaderByteCode));
	auto&& reflection = dxc.RetrieveShaderDescriptionFromByteCode(newShader.shaderByteCode);
	D3D12_SHADER_DESC shaderDesc;
	assert(SUCCEEDED(reflection->GetDesc(&shaderDesc)));
	/** 分析出该Shader中会被使用的参数，名称以及绑定方式 */
	/** Shader里面的参数对应关系最后会由每个具体的BasicShader中的参数列表进行组合
	 * 每个ParameterGroup都有一个顺序的参数列表，每个参数参数列表都会对应一个root signature的设置以及若个Descriptor heap设置*/

	 /** 分析constant buffer, texture以及sampler */
	for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
		D3D12_SHADER_INPUT_BIND_DESC resDesc;
		reflection->GetResourceBindingDesc(i, &resDesc);

		D3D12_DESCRIPTOR_RANGE range;
		range.BaseShaderRegister = resDesc.BindPoint;
		range.NumDescriptors = 1;
		range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		switch (resDesc.Type) {
		case D3D_SIT_CBUFFER: 
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; 
			break;
		case D3D_SIT_TBUFFER:
		case D3D_SIT_TEXTURE:
		case D3D_SIT_STRUCTURED:
		case D3D_SIT_BYTEADDRESS:
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			break;
		case D3D_SIT_SAMPLER:
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			break;
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		case D3D_SIT_UAV_RWTYPED:
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			break;
		default:
			assert(false); /**< 绝不应该到达这里！因为上面应该已经把所有情况涵盖了 */
		}
		range.RegisterSpace = resDesc.Space;
		newShader.ranges.insert(std::make_pair(resDesc.Name, range));
	}
	/** TODO: 避免相同Shader多次编译 */
	m_shaders.insert(std::make_pair(m_nextShaderHandle, std::move(newShader)));
	return m_nextShaderHandle++;
}

END_NAME_SPACE
