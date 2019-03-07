#ifndef UV_D3D12_CONFIG_H
#define UV_D3D12_CONFIG_H

#include "../DirectXHelper.h"
#include <d3d12.h>

class DXCompilerHelper;

namespace UnknownVision {
	const UINT BACK_BUFFER_COUNT = 2; /**< 交换链总共两个后台缓冲 */

	/** 获得DXC编译器的辅助对象 */
	DXCompilerHelper* GetDXCompilerHelper();
}

#endif // UV_D3D12_CONFIG_H
