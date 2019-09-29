#pragma once
#include "../UVConfig.h"
#include "../UVType.h"
#include "../Resource/RenderResource.h"
#include "CommandUnit.h"

#include <string>
#include <map>
#include <vector>
#include <array>

BEG_NAME_SPACE

class RenderDevice;

enum ShaderParameterType : uint8_t {
	SHADER_PARAMETER_TYPE_INVALID = 0,
	SHADER_PARAMETER_TYPE_CBV = 1 << 1,
	SHADER_PARAMETER_TYPE_SRV = 1 << 2,
	SHADER_PARAMETER_TYPE_UAV = 1 << 3,
	SHADER_PARAMETER_TYPE_SAMPLER = 1 << 4,
	SHADER_PARAMETER_TYPE_RTV = 1 << 5
};

struct ShaderParameter {
	ShaderParameter(const char* name = "") : name(name) {}
	std::string name;
	union {
		ShaderResourceView* srv;
		ConstantBufferView* cbv;
		RenderTargetView* rtv;
		UnorderAccessView* uav;
	};
};

/** ParameterPackageInterface中不存储Sampler */
struct ParameterPackageInterface {
	virtual std::vector<std::string> ShowParameterList() const = 0; /**< 展示这个参数包对应的参数列表 */
	virtual std::vector<ShaderParameter> GetParameterList() const = 0;
};

/** 自定义的Shader类都继承自这个抽象类，主要是提供Shader的描述信息 */
class BasicShader {
	friend class DX12RenderBackend;
public:
	BasicShader(const wchar_t* shaderFile) : m_shaderFile(shaderFile), m_handle(ShaderHandle::InvalidIndex()) {}
	virtual ~BasicShader() = default;
	ShaderHandle GetHandle() const { return m_handle; }
	/** 获得当前Shader使用的所有参数包 */
	virtual  std::vector<ParameterPackageInterface*> Pack() const = 0;
	/** 务必在构造Shader之前完成设置!! */
	void SetSamplerDescriptor(std::string name, SamplerDescriptor desc) { m_samplerDesces[name] = desc; }
	/** 索引当前设置的sampler */
	virtual std::map<std::string, SamplerDescriptor*> GetSamplerNameAndPointers() const = 0;
	virtual ShaderType GetShaderType() const = 0;
protected:
	ShaderHandle m_handle;
	const wchar_t* m_shaderFile;
	/** ParameterPackageInterface中不存储Sampler */
	std::map<std::string, SamplerDescriptor> m_samplerDesces; /**< 预设的部分(假如允许默认值)采样器的描述属性, Note: 务必在构建Shader前设置完毕 */
};

class VertexShader : public BasicShader {
public:
	ShaderType GetShaderType() const override final { return SHADER_TYPE_VERTEX_SHADER; }
};

class PixelShader : public BasicShader {
public:
	ShaderType GetShaderType() const override final { return SHADER_TYPE_PIXEL_SHADER; }
};

END_NAME_SPACE
