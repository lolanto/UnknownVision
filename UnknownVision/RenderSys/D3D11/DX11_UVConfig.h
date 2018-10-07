#pragma once
#include "../RenderSys_UVConfig.h"
#include <d3d11.h>
#include <map>
#include <wrl.h>

#define SmartPTR Microsoft::WRL::ComPtr

namespace DX11_UV {
	struct CanvasDataSet {
		SmartPTR<ID3D11ShaderResourceView>  srv;
		SmartPTR<ID3D11RenderTargetView>		rtv;
		SmartPTR<ID3D11Texture2D>					tex2d;
	};

	typedef std::map<UINT, CanvasDataSet> CanvasDataContainer;
};

