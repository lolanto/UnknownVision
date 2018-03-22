#pragma once
#include <wrl.h>
#include <d3d11.h>
#include <vector>

#include "RenderTarget.h"
#include "Texture.h"

enum CanvasState {
	CS_UNKNOWN = 0,
	CS_RENDER_TARGET,
	CS_SHADER_RESOURCE,
	CS_UNORDER_ACCESS
};

class Canvas : public ITexture, public IRenderTarget, 
	public IUATarget, public IterateObject {
public:
	Canvas(float width, float height, 
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, bool genMipMap = false,
		UINT arraySize = 1);

	bool Setup(ID3D11Device*);

	// IRT
	ID3D11RenderTargetView* GetRTV();
	void UnbindRTV();
	// ISR
	ID3D11ShaderResourceView** GetSRV();
	void UnbindSRV();

	// UAT
	ID3D11UnorderedAccessView** GetUAV();
	void UnbindUAV();

	// IterateObject
	void IterFunc(ID3D11Device*, ID3D11DeviceContext*);

public:
	const float																					width;
	const float																					height;
	const DXGI_FORMAT																	format;
	const bool																					hasMipmap;
	const UINT																					arraySize;
	bool																								unorderAccess;
private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D>								m_tex2d;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>					m_renderTarget;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>				m_shaderResource;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>			m_unorderAccessTarget;
	bool																								m_hasSetup;
	CanvasState																					m_state;
};

class CanvasCubeMap : public ITexture, public IRenderTarget {
public:
	CanvasCubeMap(float size, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
	
	bool Setup(ID3D11Device*);

	// IRT
	ID3D11RenderTargetView* GetRTV();
	void UnbindRTV();
	// ISR
	ID3D11ShaderResourceView** GetSRV();
	void UnbindSRV();

public:
	const float																					size;
	const DXGI_FORMAT																	format;
private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D>								m_tex2d;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>					m_renderTarget;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>				m_shaderResource;
	bool																								m_hasSetup;
	CanvasState																					m_state;
};

typedef std::vector<Canvas*> CanvasList;
