#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <string>

enum ShaderBindTarget {
	SBT_UNKNOWN = 0,
	SBT_VERTEX_SHADER,
	SBT_PIXEL_SHADER,
	SBT_GEOMETRY_SHADER,
	SBT_COMPUTE_SHADER
};

struct BufferVariableDesc {
	std::string																						name;
	UINT																							size;
	UINT																							offset;
	D3D_SHADER_VARIABLE_CLASS													varClass;
	D3D_SHADER_VARIABLE_TYPE														varType;
};

struct ConstBufferDesc {
	std::string																						name;
	UINT																							size;
	UINT																							slot;
	std::vector<BufferVariableDesc>													variables;
};

struct TextureDesc {
	std::string																						name;
	UINT																							slot;
};

struct ParamIOLayout {
	std::string																						name;
	UINT																							index;
};

class UnknownObject {
public:
	static UINT NextAssetID;
	static UINT GetNextAssetID();
public:
	UnknownObject();
	const UINT AssetID;
public:
	virtual bool Setup(ID3D11Device*) = 0;
	virtual void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T) = 0;
	virtual void Unbind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T) = 0;
};

class IterateObject {
public:
	virtual void IterFunc(ID3D11Device* dev, ID3D11DeviceContext*) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Utility Utility   /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

// 检查两个constantBuffer的描述是否匹配(用来校验当前绑定的缓冲区和缓冲区接口是否一致)
bool ConstantBuffersTest(ConstBufferDesc* cbd1, ConstBufferDesc* cbd2);
