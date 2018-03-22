#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXTex.h>
#include <memory>

class ITexture {
public:
	virtual ID3D11ShaderResourceView** GetSRV() = 0;
	virtual bool Setup(ID3D11Device*) = 0;
};

class CommonTexture : public ITexture {
public:
	CommonTexture(const wchar_t* file);

public:
	bool Setup(ID3D11Device* dev);
	ID3D11ShaderResourceView** GetSRV();

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D>						m_tex2d;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_shaderResource;
	const wchar_t*																		m_source;
};

class HDRTexture : public ITexture {
public:
	HDRTexture(const wchar_t* file);

public:
	bool Setup(ID3D11Device* dev);
	ID3D11ShaderResourceView** GetSRV();

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D>						m_tex2d;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_shaderResource;
	const wchar_t*																		m_source;
};

class InternalTexture : public ITexture {
public:
	InternalTexture(ID3D11ShaderResourceView* srv);
public:
	bool Setup(ID3D11Device* dev);
	ID3D11ShaderResourceView** GetSRV();
private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_shaderResource;
};

class DepthTexture : public ITexture {
public:
	DepthTexture(float width, float height, UINT arraySize = 1,
		DXGI_FORMAT bufFormat = DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT depFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
		DXGI_FORMAT resFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
public:
	bool Setup(ID3D11Device* dev);
	ID3D11DepthStencilView* GetDSV();
	ID3D11ShaderResourceView** GetSRV();

	void SetCubeMap();
	void SetMipMap();

	const DXGI_FORMAT															bufFormat;
	const DXGI_FORMAT															depFormat;
	const DXGI_FORMAT															resFormat;
	const float																			width;
	const float																			height;
private:
	UINT																					m_arraySize;
	bool																						m_cubemap;
	bool																						m_mipmap;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>			m_depStenView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_shaderResource;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>						m_texture;
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
private:
	TextureFactory();
};
