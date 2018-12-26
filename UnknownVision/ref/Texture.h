#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXTex.h>
#include <memory>
#include "Resource.h"

/*
仅加载外部贴图文件生成相关贴图资源
*/

// 通过外部文件加载贴图——只读
class CommonTexture : public ITexture {
public:
	CommonTexture(const wchar_t* file);

public:
	bool Setup(ID3D11Device* dev);

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D>						m_tex2d;
	const wchar_t*																	m_source;
};

// 通过外部文件加载HDR文件——只读
class HDRTexture : public ITexture {
public:
	HDRTexture(const wchar_t* file);

public:
	bool Setup(ID3D11Device* dev);

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D>						m_tex2d;
	const wchar_t*																		m_source;
};

// 通过外部文件加载dds文件——只读
class DDSTexture : public ITexture {
public:
	DDSTexture(const wchar_t* file);

public:
	bool Setup(ID3D11Device* dev);
private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D>						m_tex2d;
	const wchar_t*																		m_source;
};

class DDSTextureArray : public ITexture {
public:
	template<typename... Args>
	DDSTextureArray(const wchar_t* path, Args... args) {
		reader(path, args...);
	}

	//DDSTextureArray(const wchar_t* path) {
	//	m_sources.push_back(path);
	//}

	template<typename... Args>
	void reader(const wchar_t* path, Args... args) {
		m_sources.push_back(path);
		reader(args...);
	}

	void reader(const wchar_t* path) {
		m_sources.push_back(path);
	}

public:
	bool Setup(ID3D11Device* dev);
private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D>						m_tex2d;
	std::vector<const wchar_t*>												m_sources;
};

class DepthTexture : public ITexture, public IDepthStencil {
public:
	DepthTexture(float width, float height, bool hasMipMap = false, UINT arraySize = 1,
		DXGI_FORMAT bufFormat = DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT depFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
		DXGI_FORMAT resFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
public:
	virtual bool Setup(ID3D11Device* dev);

	const DXGI_FORMAT															bufFormat;
	const DXGI_FORMAT															depFormat;
	const DXGI_FORMAT															resFormat;
	const float																			width;
	const float																			height;
	const bool																			hasMipMap;
protected:
	virtual void preSetDesc();
protected:
	UINT																					m_arraySize;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>						m_texture;
	D3D11_TEXTURE2D_DESC													m_texDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC									m_dsDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC							m_rsvDesc;
};

class CubeMapDepthTexture : public DepthTexture {
public:
	CubeMapDepthTexture(float size, bool hasMipMap = false,
		DXGI_FORMAT bufFormat = DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT depFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
		DXGI_FORMAT resFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
public:
	virtual bool Setup(ID3D11Device* dev);
protected:
	virtual void preSetDesc();
};

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   TextureFactory   ///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

class TextureFactory {
public:
	static TextureFactory& GetInstance();

public:
	bool LoadCommonTextureFromFile(const wchar_t* file, std::shared_ptr<DirectX::ScratchImage>&);
	bool LoadHDRTextureFromFile(const wchar_t* file, std::shared_ptr<DirectX::ScratchImage>&);
	bool LoadDDSTextureFromFile(const wchar_t* file, std::shared_ptr<DirectX::ScratchImage>&);
private:
	TextureFactory();
};
