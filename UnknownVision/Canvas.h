#pragma once
#include <wrl.h>
#include <d3d11.h>
#include <vector>

#include "Resource.h"

class Canvas : public ITexture, public IRenderTarget, public IUnorderAccess
{
public:
	Canvas(float width, float height, 
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

	virtual bool Setup(ID3D11Device*);
public:
	const DXGI_FORMAT																	format;
	const float																					width;
	const float																					height;
	void SetMipmap(bool v = true);
	void GenMipMap(ID3D11DeviceContext*);
	void GenArray(UINT size = 6);
	void SetUARes(bool v = true, bool simRW = false);
	ID3D11Texture2D* GetTex();
private:
	// 由于DX11.0的UAV支持读写的格式有限，故需要一些转换工作
	bool formatCheckForUARes();
protected:
	virtual void preSetDesc();
protected:
	bool																								m_hasMipmap;
	UINT																							m_arraySize;
	bool																								m_isUnorderAccess;
	bool																								m_simultaneouslyRW;
protected:
	D3D11_TEXTURE2D_DESC															m_texDesc;
	D3D11_RENDER_TARGET_VIEW_DESC											m_rtvDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC										m_srvDesc;
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

