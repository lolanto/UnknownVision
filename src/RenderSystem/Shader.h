#pragma once
#include "../UVConfig.h"
#include "../UVType.h"

#include <string>
#include <map>
#include <array>

BEG_NAME_SPACE

#define GetGroup1(ShaderType) ShaderType::Group1
#define GetGroup2(ShaderType) ShaderType::Group2
#define GetGroup3(ShaderType) ShaderType::Group3
#define GetGroup4(ShaderType) ShaderType::Group4
#define GetGroup5(ShaderType) ShaderType::Group5

constexpr int MAX_PARAMETER_GROUP = 5;

class RenderDevice;

enum ShaderParameterType : uint8_t {
	SHADER_PARAMETER_TYPE_INVALID = 0,
	SHADER_PARAMETER_TYPE_CBV = 1,
	SHADER_PARAMETER_TYPE_SRV = 2,
	SHADER_PARAMETER_TYPE_UAV = 3,
	SHADER_PARAMETER_TYPE_SAMPLER = 4,
	SHADER_PARAMETER_TYPE_RTV = 5
};

struct ShaderParameter {
	ShaderParameter() : type(SHADER_PARAMETER_TYPE_INVALID), registerIndex(UINT8_MAX), space(UINT8_MAX) {}
	union {
		TextureHandle texture;
		BufferHandle buffer;
	};
	ShaderParameter& operator=(TextureHandle handle) { texture = handle; return *this; }
	ShaderParameter& operator=(BufferHandle handle) { buffer = handle; return *this; }
	ShaderParameter& operator=(const ShaderParameter& rhs) {
		memcpy(this, &rhs, sizeof(ShaderParameter));
		return *this;
	}
	ShaderParameterType type;
	uint8_t registerIndex;
	uint8_t space;
};

struct ShaderParameterGroup {
	ShaderParameterGroup() = default;
	ShaderParameter operator[](const char* name) const { return parameters[name]; }
	ShaderParameter& operator[](const char* name) { return parameters[name]; }
	size_t Size() const { return parameters.size(); }
	void swap(ShaderParameterGroup& rhs) {
		parameters.swap(rhs.parameters);
	}
	std::map<std::string, ShaderParameter> parameters;
};

using ARR_ShaderParameterGroup = std::array<ShaderParameterGroup, MAX_PARAMETER_GROUP>;

class ShaderInterface {
public:
	ShaderHandle GetHandle() const { return m_handle; }
	virtual std::string GetSourceCode() const = 0;
protected:
	ShaderHandle m_handle;
};

END_NAME_SPACE
