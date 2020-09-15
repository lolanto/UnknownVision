#pragma once
#include "../UVConfig.h"
#include "../UVType.h"
#include "../GPUResource/GPUResource.h"
#include "CommandUnit.h"

#include <string>
#include <map>
#include <vector>
#include <array>
#include <functional>

BEG_NAME_SPACE

class RenderDevice;

/** 描述Shader中每一个参数的绑定属性 */
struct ShaderParameterSlotDesc {
	ShaderParameterType paramType;
	uint8_t slot;
	uint8_t count;
	uint8_t space;
	std::function<SamplerDescriptor()> samplerDesc;
	ShaderParameterSlotDesc() = default;
	static ShaderParameterSlotDesc OnlyReadTexture(uint8_t slot_, uint8_t count_, uint8_t space_ = 0) {
		ShaderParameterSlotDesc desc;
		desc.paramType = SHADER_PARAMETER_TYPE_TEXTURE_R;
		desc.slot = slot_;
		desc.count = count_;
		desc.space = space_;
		return desc;
	}
	static ShaderParameterSlotDesc OnlyReadBuffer(uint8_t slot_, uint8_t count_, uint8_t space_ = 0) {
		ShaderParameterSlotDesc desc;
		desc.paramType = SHADER_PARAMETER_TYPE_BUFFER_R;
		desc.slot = slot_;
		desc.count = count_;
		desc.space = space_;
		return desc;
	}
	static ShaderParameterSlotDesc LinearSampler(uint8_t slot_, uint8_t count_ = 1, uint8_t space_ = 0) {
		ShaderParameterSlotDesc desc;
		desc.paramType = SHADER_PARAMETER_TYPE_SAMPLER;
		desc.slot = slot_;
		desc.count = count_;
		desc.space = space_;
		desc.samplerDesc = []() {
			return SamplerDescriptor(FILTER_TYPE_MIN_MAG_MIP_LINEAR,
				SAMPLER_ADDRESS_MODE_WRAP,
				SAMPLER_ADDRESS_MODE_WRAP,
				SAMPLER_ADDRESS_MODE_WRAP);
		};
		return desc;
	}
};

/** 自定义的Shader类都继承自这个抽象类，存储索引shader实体的handle
 * 实际使用时由Device/Backend通过handle索引Shader平台相关的数据 */
class BasicShader {
	friend class DX12RenderBackend;
public:
	BasicShader(const wchar_t* shaderFile) : m_shaderFile(shaderFile),m_srcCode(nullptr),  m_handle(ShaderHandle::InvalidIndex()) {}
	BasicShader(const char* srcCode) : m_shaderFile(nullptr), m_srcCode(srcCode), m_handle(ShaderHandle::InvalidIndex()) {}
	virtual ~BasicShader() = default;
	ShaderHandle GetHandle() const { return m_handle; }
	virtual ShaderType GetShaderType() const = 0;
	virtual std::vector<std::vector<ShaderParameterSlotDesc>> GetShaderParameters() const { return { {} }; }
	virtual const char* Name() const { return nullptr; }
protected:
	ShaderHandle m_handle;
	const wchar_t* m_shaderFile;
	const char* m_srcCode;
};

class VertexShader : public BasicShader {
public:
	VertexShader(const wchar_t* shaderFile) : BasicShader(shaderFile) {}
	VertexShader(const char* srcCode) : BasicShader(srcCode) {}
public:
	ShaderType GetShaderType() const override final { return SHADER_TYPE_VERTEX_SHADER; }
};

class PixelShader : public BasicShader {
public:
	PixelShader(const wchar_t* shaderFile) : BasicShader(shaderFile) {}
	PixelShader(const char* srcCode) : BasicShader(srcCode) {}
public:
	ShaderType GetShaderType() const override final { return SHADER_TYPE_PIXEL_SHADER; }
};

END_NAME_SPACE
