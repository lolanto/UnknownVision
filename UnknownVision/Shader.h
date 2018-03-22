#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>

#include "UnknownObject.h"

const UINT FREQUENCY = 1;

class IShader {
public:
	static void ShaderFactory(std::string& filePath,
		Microsoft::WRL::ComPtr<ID3DBlob>& blob,
		LPCSTR target);
public:
	virtual bool Setup(ID3D11Device*, ID3D11DeviceContext*) = 0;
	virtual void Bind(ID3D11Device*, ID3D11DeviceContext*) = 0;
	virtual void Unbind(ID3D11Device*, ID3D11DeviceContext*) = 0;
	virtual LPVOID GetByteCode() = 0;
	virtual size_t GetByteCodeSize() = 0;
};

struct ShaderConstBufVarDesc {
	std::string											name;
	UINT												size;
	UINT												offset;
	D3D_SHADER_VARIABLE_CLASS		varClass;
	D3D_SHADER_VARIABLE_TYPE			varType;
};

struct ShaderConstBufDesc {
	std::vector<ShaderConstBufVarDesc>					variables;
	UINT																	slot;
	UINT																	size;
};

// shader分析器
class ShaderAnalyser {
public:
	ShaderAnalyser(IShader*);
	void Analyse(std::vector<ShaderConstBufDesc>&);
private:
	void analyseConstantBuffer(UINT index, ShaderConstBufDesc&);
	void analyseCBVariable(UINT index, ID3D11ShaderReflectionVariable*&, ShaderConstBufVarDesc&);
private:
	// 分析的对象
	IShader*																						m_target;
	
	Microsoft::WRL::ComPtr<ID3D11ShaderReflection>						m_reflection;
	D3D11_SHADER_DESC																	m_shaderDesc;
};

class VertexShader : public IShader {
public:
	VertexShader(std::string filePath);
	bool Setup(ID3D11Device*, ID3D11DeviceContext*);
	void Bind(ID3D11Device*, ID3D11DeviceContext*);
	void Unbind(ID3D11Device*, ID3D11DeviceContext*);
	LPVOID GetByteCode();
	size_t GetByteCodeSize();
private:
	Microsoft::WRL::ComPtr<ID3DBlob>											m_codeBlob;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>							m_shader;
};

class PixelShader : public IShader {
public:
	PixelShader(std::string filePath);
	bool Setup(ID3D11Device*, ID3D11DeviceContext*);
	void Bind(ID3D11Device*, ID3D11DeviceContext*);
	void Unbind(ID3D11Device*, ID3D11DeviceContext*);
	LPVOID GetByteCode();
	size_t GetByteCodeSize();
private:
	Microsoft::WRL::ComPtr<ID3DBlob>											m_codeBlob;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>								m_shader;
};

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   update   ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

struct ShaderDesc {
	UINT																							constBufferNum;
	std::vector<ConstBufferDesc>														constBuffers;

	UINT																							textureNum;
	std::vector<TextureDesc>															textures;
};

class ShaderObject : public UnknownObject {
public:
	Microsoft::WRL::ComPtr<ID3DBlob>											m_codeBlob;
	virtual ShaderDesc* GetDescription();
	virtual ConstBufferDesc* GetShaderCBuffer(UINT);
	virtual bool Setup(ID3D11Device*) = 0;
	virtual void Bind(ID3D11DeviceContext*) = 0;
	virtual void Unbind(ID3D11DeviceContext*) = 0;
protected:
	ShaderDesc																					m_desc;
};

class VertexShader1 : public ShaderObject {
public:
	VertexShader1(std::string filePath);
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);
	// 简单的输入结构的描述，只用做校验
	std::vector<ParamIOLayout>* GetInputLayout();
private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader>							m_shader;
	std::vector<ParamIOLayout>														m_inputLayout;
};

class PixelShader1 : public ShaderObject {
public:
	PixelShader1(std::string filePath);
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);
	std::vector<ParamIOLayout>* GetOutputLayout();
private:
	Microsoft::WRL::ComPtr<ID3D11PixelShader>								m_shader;
	std::vector<ParamIOLayout>														m_outputLayout;
};

class GeometryShader1 : public ShaderObject {
public:
	GeometryShader1(std::string filePath);
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);
private:
	Microsoft::WRL::ComPtr<ID3D11GeometryShader>						m_shader;
};

class ComputeShader1 : public ShaderObject {
public:
	ComputeShader1(std::string filePath);
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);
private:
	Microsoft::WRL::ComPtr<ID3D11ComputeShader>						m_shader;
};
