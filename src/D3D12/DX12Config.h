#pragma once

#include "../UVConfig.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <d3d12.h>
#include <tuple>
#include <wrl.h>

class DXCompilerHelper;
class WindowWin32;
using DX12BackendUsedData = std::tuple<const WindowWin32* const, uint32_t, uint32_t>;
#define SmartPTR Microsoft::WRL::ComPtr

BEG_NAME_SPACE
const UINT NUMBER_OF_BACK_BUFFERS = 2; /**< 交换链总共两个后台缓冲 */

END_NAME_SPACE

