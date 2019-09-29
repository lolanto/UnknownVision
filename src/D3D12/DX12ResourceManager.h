#pragma once

#include "DX12Config.h"
#include "DX12HeapManager.h"
#include <D3D12MemAlloc.h> /** 重新修改资源管理器，使用新的内存管理器，再对资源申请进行调整，适应新的管理方式 */
#include <map>
#include <mutex>
#include <atomic>

BEG_NAME_SPACE
/** 创建committed类型Buffer的起始大小，要么是2M，要么是堆块的一半，取小的一半 */
constexpr static uint32_t BUFFER_SIZE_USED_COMMITTED_RESOURCE = 
	(1u << 21) > (HEAP_CHUNK_SIZE / 2) ? (HEAP_CHUNK_SIZE / 2) : (1u << 21);
/** 假如缓冲区大于等于该值，则一旦不被使用就应该立即释放 暂时定为heap管理的堆大小的2/3 */
constexpr static uint32_t BUFFER_SIZE_RELEASE_IMMEDIATELY = (HEAP_CHUNK_SIZE / 3) * 2;

/** 创建committed类型Texture的起始大小，要么是4M，要么是堆块的一半，取小的一半 */
constexpr static uint32_t TEXTURE_SIZE_USED_COMMITTED_RESOURCE =
(1u << 22) > (HEAP_CHUNK_SIZE / 2) ? (HEAP_CHUNK_SIZE / 2) : (1u << 22);
/** 假如缓冲区大于等于该值，则一旦不被使用就应该立即释放 暂时定为heap管理的堆大小的2/3 */
constexpr static uint32_t TEXTURE_SIZE_RELEASE_IMMEDIATELY = (HEAP_CHUNK_SIZE / 3) * 2;

class DX12ResourceManager {
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
	/** 禁止非右值传递 */
	~DX12ResourceManager() = default; /**< 假如有资源正在被引用，可能会造成错误 */
	DX12ResourceManager(const DX12ResourceManager&) = delete;
	DX12ResourceManager(DX12ResourceManager&& rhs) { swapWithRValue(std::move(rhs)); }
	DX12ResourceManager& operator=(const DX12ResourceManager&) = delete;
	DX12ResourceManager& operator=(DX12ResourceManager&& rhs) { swapWithRValue(std::move(rhs)); return *this; }

public:
	bool Initialize();
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
 * @param heapType 资源存储的堆类型
 * @mipLevel 纹理资源的细分等级，0表示创建长宽可创建的细分上限
 * @bForceCommitted 强制要求创建CommittedResource
 * @return 创建成功返回纹理指针以及资源当前的状态，创建失败返回nullptr以及D3D12_RESOURCE_STATE_COMMON
 * @remark slice不为0时创建三维纹理，height不为0时创建二维纹理，width不为0时创建一维纹理
 * 不再提供heapType设置，因为texture只允许创建在default类型的heap上 */
	auto RequestTexture(uint32_t width, uint32_t height, uint32_t slice, DXGI_FORMAT elementFormat,
		D3D12_RESOURCE_FLAGS flags,
		uint16_t mipLevel, bool bForceCommitted)->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES>;
	/** 向管理器归还资源
 * @param res 之前请求的纹理的指针
 * @return 归还成功返回true, 失败返回false */
	bool ReleaseResource(ID3D12Resource*);
private:
	inline void swapWithRValue(DX12ResourceManager&& rvalue);
private:
	D3D12MA::Allocator* m_pAllocator;
	ID3D12Device* m_pDevice;
	std::map<ID3D12Resource*, ResourceInfo> m_resourceRepository;
};

END_NAME_SPACE
