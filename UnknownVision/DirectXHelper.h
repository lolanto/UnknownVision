#ifndef DIRECTX_HELPER_H
#define DIRECTX_HELPER_H
/** 该文件包含DirectX通用的辅助函数 */

#include "UVConfig.h"
#include <dxgi1_6.h>
#include <cassert>
#include <wrl.h>

#define SmartPTR Microsoft::WRL::ComPtr

namespace UnknownVision {
	/** 将自定义的DataFormatType转换成DX可读的DXGI格式 */
	inline DXGI_FORMAT DataFormatType2DXGI_FORMAT(const DataFormatType& type) {
		switch (type) {
		case DFT_D24_UNORM_S8_UINT:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case DFT_INVALID:
			assert(false);
			break;
		case DFT_R32G32B32A32_FLOAT:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case DFT_R32G32B32_FLOAT:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case DFT_R32G32_FLOAT:
			return DXGI_FORMAT_R32G32_FLOAT;
		case DFT_R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case DFT_R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		return DXGI_FORMAT_UNKNOWN;
	}

}

#endif // !DIRECTX_HELPER
