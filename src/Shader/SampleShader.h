#pragma once
#include "../RenderSystem/Shader.h"

BEG_NAME_SPACE

class SampleShaderVS : public ShaderInterface {
public:
	SampleShaderVS() : ShaderInterface("hellovs") {
		m_mvp.name = "mvp";
	}

	virtual size_t GetNumParameters(ShaderParameterType type) const override final {
		uint8_t paraType = static_cast<uint8_t>(type);
		size_t res = 0;
		if (paraType & SHADER_PARAMETER_TYPE_CBV) res += 1;
		return res;
	}
	virtual std::vector<ParameterPackage> Pack() const override final {
		std::vector<ParameterPackage> packages;
		ParameterPackage pack;
		pack.PackBack(m_mvp);
		packages.push_back(pack);
		return packages;
	}
	void SetParameters(Buffer* mvp) { m_mvp.buffer = mvp; }
private:
	ShaderParameter m_mvp;
	std::string m_sourceCode;
};

class SampleShaderPS : public ShaderInterface {
public:
	SampleShaderPS() : ShaderInterface("hellops") {
		m_tex.name = "tex";
	}
	virtual size_t GetNumParameters(ShaderParameterType type) const override final {
		uint8_t paraType = static_cast<uint8_t>(type);
		size_t res = 0;
		if (paraType & SHADER_PARAMETER_TYPE_SRV) res += 1;
		return res;
	}
	virtual std::vector<ParameterPackage> Pack() const override final {
		std::vector<ParameterPackage> packages;
		ParameterPackage pack;
		pack.PackBack(m_tex);
		packages.push_back(pack);
		return packages;
	}
	void SetParameters(Texture2D* tex) { m_tex.texture2d = tex; }
private:
	ShaderParameter m_tex;
	std::string m_sourceCode;
};

END_NAME_SPACE
