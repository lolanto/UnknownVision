#pragma once
#include <wrl.h>
#include <d3d11.h>
#include <vector>

#include "Resource.h"

class Canvas : public ITexture, public IRenderTarget, public IUnorderAccess
{
public:
	Canvas(float width, float height, 
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, 
		bool unorderAccess = false,
		bool genMipMap = false,
		UINT arraySize = 1);

	virtual bool Setup(ID3D11Device*);
public:
	const float																					width;
	const float																					height;
	const DXGI_FORMAT																	format;
	const bool																					hasMipmap;
	const UINT																					arraySize;
	const bool																					isUnorderAccess;
protected:
	virtual void preSetDesc();
protected:
	D3D11_TEXTURE2D_DESC															m_texDesc;
	D3D11_RENDER_TARGET_VIEW_DESC											m_rtvDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC									m_srvDesc;
	D3D11_UNORDERED_ACCESS_VIEW_DESC									m_uavDesc;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>								m_tex2d;
};

class CanvasCubeMap : public Canvas {
public:
	CanvasCubeMap(float size, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
	
	virtual bool Setup(ID3D11Device*);
protected:
	virtual void preSetDesc();
};

