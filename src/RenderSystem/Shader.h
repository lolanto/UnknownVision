#pragma once
#include "../UVConfig.h"
#include "../UVType.h"
#include "RenderDescriptor.h"
#include "CommandUnit.h"

#include <string>
#include <map>
#include <vector>
#include <array>

BEG_NAME_SPACE

#define GetGroup1(ShaderType) ShaderType::Group1
#define GetGroup2(ShaderType) ShaderType::Group2
#define GetGroup3(ShaderType) ShaderType::Group3
#define GetGroup4(ShaderType) ShaderType::Group4
#define GetGroup5(ShaderType) ShaderType::Group5

constexpr int MAX_PARAMETER_GROUP = 5;

class RenderDevice;
class Buffer;
class Texture2D;

enum ShaderParameterType : uint8_t {
	SHADER_PARAMETER_TYPE_INVALID = 0,
	SHADER_PARAMETER_TYPE_CBV = 1 << 1,
	SHADER_PARAMETER_TYPE_SRV = 1 << 2,
	SHADER_PARAMETER_TYPE_UAV = 1 << 3,
	SHADER_PARAMETER_TYPE_SAMPLER = 1 << 4,
	SHADER_PARAMETER_TYPE_RTV = 1 << 5
};

struct ShaderParameter {
	ShaderParameter(const char* name = "") : name(name), buffer(nullptr) {}
	std::string name;
	union {
		Texture2D* texture2d;
		Buffer* buffer;
	};
};

class DX12RenderBackend;

/** 自定义的Shader类都继承自这个抽象类，主要是提供Shader的描述信息 */
class BasicShader {
public:
	BasicShader(const char* shaderFile) : m_shaderFile(shaderFile) {}
	virtual ~BasicShader() = default;
	ShaderHandle GetHandle() const { return m_handle; }
	/** 返回该Shader某个类型参数的数量 */
	virtual size_t GetNumParameters(ShaderParameterType type) const = 0;
	/** 绑定参数 */
	virtual  std::vector<ParameterPackageInterface> Pack() const = 0;
	/** 务必在构造Shader之前完成设置!! */
	void SetSamplerDescriptor(std::string name, SamplerDescriptor desc) { m_samplerDesces[name] = desc; }
protected:
	ShaderHandle m_handle;
	const char* m_shaderFile;
	std::map<std::string, SamplerDescriptor> m_samplerDesces; /**< 预设的部分(假如允许默认值)采样器的描述属性, Note: 务必在构建Shader前设置完毕 */
};

struct ParameterPackageInterface {
	virtual std::vector<std::string> ShowParameterList() const = 0; /**< 展示这个参数包对应的参数列表 */
	virtual std::vector<ShaderParameter> GetParameterList() const = 0;
};

/** 负责将参数传送到GPU  */
class ParameterPipeline {
public:
	virtual bool SendPackage(CommandUnit& cu, std::vector<ParameterPackageInterface> packages) = 0;
};

END_NAME_SPACE
