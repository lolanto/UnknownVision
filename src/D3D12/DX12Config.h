#ifndef UV_D3D12_CONFIG_H
#define UV_D3D12_CONFIG_H

#include "../DirectXHelper.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <d3d12.h>
#include <tuple>

class DXCompilerHelper;
class WindowWin32;
using BackendUsedData = std::tuple<const WindowWin32* const, uint32_t, uint32_t>;

namespace UnknownVision {
	const UINT BACK_BUFFER_COUNT = 2; /**< 交换链总共两个后台缓冲 */

	/** 获得DXC编译器的辅助对象 */
	DXCompilerHelper* GetDXCompilerHelper();

	inline D3D12_RESOURCE_STATES TransformEnum(ResourceState state) {
		switch (state) {
		case RESOURCE_STATE_COPY_DEST:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case RESOURCE_STATE_CONSTANT_BUFFER:
			return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		default:
			return D3D12_RESOURCE_STATE_COMMON;
		}
	}
}

#endif // UV_D3D12_CONFIG_H
