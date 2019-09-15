#pragma once
#include "../UVConfig.h"
#include "../UVType.h"
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

class ShaderInterface {
public:
	ShaderInterface(const char* shaderFile) : m_shaderFile(shaderFile) {}
	virtual ~ShaderInterface() = default;
	ShaderHandle GetHandle() const { return m_handle; }
	/** 返回该Shader某个类型参数的数量 */
	virtual size_t GetNumParameters(ShaderParameterType type) const = 0;
	/** 绑定参数 */
	virtual  std::vector<ParameterPackage> Pack() const = 0;
protected:
	ShaderHandle m_handle;
	const char* m_shaderFile;
};

struct ParameterPackage {
	bool IsEmpty() const { return parameters.empty(); }
	size_t GetCapacity() const { return parameters.size(); }
	void Pack(ShaderParameter cargo, size_t position) { parameters[position] = cargo; }
	void PackBack(ShaderParameter cargo) { parameters.push_back(cargo); }
	size_t uniqueCode;
	std::vector<ShaderParameter> parameters;
};

/** 负责将参数传送到GPU  */
class ParameterPipeline {
public:
	virtual bool SendPackage(CommandUnit& cu, std::vector<ParameterPackage> packages) = 0;
};

END_NAME_SPACE
