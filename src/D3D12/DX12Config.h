#pragma once

#include "../UVConfig.h"
#include "../UVUtility.h"
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
const DXGI_FORMAT BACK_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr uint8_t NUMBER_OF_DESCRIPTOR_IN_RTV_HEAP = 32;
constexpr uint8_t NUMBER_OF_DESCRIPTOR_IN_DSV_HEAP = 32;
constexpr uint32_t NUMBER_OF_DESCRIPTOR_IN_EACH_DESCRIPTOR_HEAP[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
	50, 30, MAX_RENDER_TARGET, MAX_RENDER_TARGET
	/** SRV_CBV_UAV, sampler, RTV, DSV */
};

END_NAME_SPACE

