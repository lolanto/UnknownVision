#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>

#include "UnknownObject.h"

const UINT FREQUENCY = 1;

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

class VertexShader : public ShaderObject {
public:
	VertexShader(std::string filePath);
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);
	// �򵥵�����ṹ��������ֻ����У��
	std::vector<ParamIOLayout>* GetInputLayout();
private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader>						m_shader;
	std::vector<ParamIOLayout>														m_inputLayout;
};

class PixelShader : public ShaderObject {
public:
	PixelShader(std::string filePath);
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);
	std::vector<ParamIOLayout>* GetOutputLayout();
private:
	Microsoft::WRL::ComPtr<ID3D11PixelShader>								m_shader;
	std::vector<ParamIOLayout>														m_outputLayout;
};

class GeometryShader : public ShaderObject {
public:
	GeometryShader(std::string filePath);
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);
private:
	Microsoft::WRL::ComPtr<ID3D11GeometryShader>						m_shader;
};

class ComputeShader : public ShaderObject {
public:
	ComputeShader(std::string filePath);
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);
private:
	Microsoft::WRL::ComPtr<ID3D11ComputeShader>						m_shader;
};
