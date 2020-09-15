#pragma once
#include "../UVConfig.h"
#include "../UVUtility.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <d3d12.h>
#include <tuple>
#include <wrl.h>

class DXCompilerHelper;
#define SmartPTR Microsoft::WRL::ComPtr

BEG_NAME_SPACE
const DXGI_FORMAT BACK_BUFFER_FORMAT = DXGI_FORMAT_R16G16B16A16_FLOAT; /**< TODO: 应该有一处位置统一backbuffer的格式 */
constexpr uint8_t NUMBER_OF_DESCRIPTOR_IN_RTV_HEAP = 32;
constexpr uint8_t NUMBER_OF_DESCRIPTOR_IN_DSV_HEAP = 32;
constexpr uint32_t NUMBER_OF_DESCRIPTOR_IN_EACH_DESCRIPTOR_HEAP[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
	50, 30, MAX_RENDER_TARGET, MAX_RENDER_TARGET
	/** SRV_CBV_UAV, sampler, RTV, DSV */
};

#define VS_PROFILE "vs_5_1"
#define PS_PROFILE "ps_5_1"

struct AllocateRange {
	static AllocateRange INVALID() { AllocateRange range; range.beg = 1; range.end = 0; return range; }
	size_t beg;
	size_t end;
	size_t AllocatedSize() const { return end - beg + 1; }
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	bool operator==(const AllocateRange& rhs) { return beg == rhs.beg && end == rhs.end; }
	bool Valid() const { return end >= beg; }
};

END_NAME_SPACE

