#ifndef D3D11_UV_CONFIG_H
#define D3D11_UV_CONFIG_H
#include "../ResMgr/ResMgr_UVConfig.h"
#include "../ResMgr/Resource.h"
#include <d3d11.h>
#include <wrl.h>
#include <cassert>

#define SmartPTR Microsoft::WRL::ComPtr

namespace UnknownVision {
	inline DXGI_FORMAT TextureElementType2DXGI_FORMAT(const TextureElementType& type) {
		switch (type) {
		case UNORM_R8G8B8: // 当读取24位像素时，每个像素需要扩展8位
		case UNORM_R8G8B8A8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case UNORM_D24_UINT_S8:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		default:
			assert(false);
			break;
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	inline DXGI_FORMAT VertexAttributeDataType2DXGI_FORMAT(const VertexAttributeDataType& vadt) {
		switch (vadt) {
		case VADT_FLOAT1:
			return DXGI_FORMAT_R32_FLOAT;
			break;
		case VADT_FLOAT2:
			return DXGI_FORMAT_R32G32_FLOAT;
			break;
		case VADT_FLOAT3:
			return DXGI_FORMAT_R32G32B32_FLOAT;
			break;
		case VADT_FLOAT4:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
			break;
		default:
			assert(false);
			break;
		}
		return DXGI_FORMAT_UNKNOWN;
	}
}

#endif // D3D11_UV_COFING_H
