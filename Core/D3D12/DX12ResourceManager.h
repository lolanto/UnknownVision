#pragma once

#include "DX12Config.h"
#include <DirectXMemoryAllocator/D3D12MemAlloc.h>/** 重新修改资源管理器，使用新的内存管理器，再对资源申请进行调整，适应新的管理方式 */
#include <map>
#include <mutex>
#include <atomic>

BEG_NAME_SPACE

class DX12ResourceManager : public Uncopyable {
public:
	/** 分配出去的资源块 */
	struct ResourceInfo {
		D3D12MA::Allocation* pAllocation;
		ID3D12Resource* pResource;
		D3D12_RESOURCE_STATES state;
		ResourceInfo(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON)
			: pAllocation(nullptr), pResource(nullptr), state(state) {}
	};
public:
	DX12ResourceManager(ID3D12Device* device) 
		: m_pDevice(device), m_pAllocator(nullptr) {}
	~DX12ResourceManager();
	/** 禁止非右值传递 */
	DX12ResourceManager(const DX12ResourceManager& rhs) = delete;
	DX12ResourceManager& operator=(const DX12ResourceManager& rhs) = delete;
	DX12ResourceManager(DX12ResourceManager&& rhs) { swapWithRValue(std::move(rhs)); }
	DX12ResourceManager& operator=(DX12ResourceManager&& rhs) { swapWithRValue(std::move(rhs)); return *this; }

public:
	bool Initialize(IDXGIAdapter* adapter);
	/** 根据请求返回一个满足要求且可用的buffer资源
 * @param size 请求的buffer的大小，单位字节
 * @param flag 请求的buffer的特性(用途)
 * @param heapType 请求的buffer所在的堆类型
 * @param bForceCommitted 是否强制要求使用CommittedResource
 * @return 创建成功返回buffer的指针以及当前的初始状态；创建失败返回{NULL, D3D12_RESOURCE_STATE_COMMON}
 * @remark 返回的state可能会有很多种！需要进行一定的状态切换*/
	auto RequestBuffer(size_t size, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType,
		bool bForceCommitted = false)
		->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES>;
/** TODO: 向管理器请求一个满足要求，且可用的纹理资源
 * @param width, height, slice 请求的纹理的长宽，单位像素
 * @param elementFormat 纹理像素的格式
 * @param flags 纹理资源的特性(用途)
 * @param arraySize 创建纹理数组时使用，仅对1D和2D纹理有效
 * @mipLevel 纹理资源的细分等级，0表示创建长宽可创建的细分上限
 * @bForceCommitted 强制要求创建CommittedResource
 * @return 创建成功返回纹理指针以及资源当前的状态，创建失败返回nullptr以及D3D12_RESOURCE_STATE_COMMON
 * @remark slice不为0时创建三维纹理，height不为0时创建二维纹理，width不为0时创建一维纹理
 * 不再提供heapType设置，因为texture只允许创建在default类型的heap上 */
	auto RequestTexture(uint32_t width, uint32_t height, uint32_t slice, DXGI_FORMAT elementFormat,
		D3D12_RESOURCE_FLAGS flags, uint16_t arraySize,
		uint16_t mipLevel, bool bForceCommitted)->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES>;
	/** 向管理器归还资源
 * @param pRes 之前请求的资源指针
 * @return 归还成功返回true, 失败返回false */
	bool ReleaseResource(ID3D12Resource* pRes);
private:
	inline void swapWithRValue(DX12ResourceManager&& rvalue);
private:
	D3D12MA::Allocator* m_pAllocator;
	ID3D12Device* m_pDevice;
	std::map<ID3D12Resource*, ResourceInfo> m_resourceRepository; /**< 所有创建的资源都存储于此 */
};

END_NAME_SPACE
