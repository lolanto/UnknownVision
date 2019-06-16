#pragma once

#include "../RenderSystem/RenderBasic.h"
#include "DX12HeapManager.h"
#include "DX12Config.h"
#include <map>
#include <mutex>
#include <atomic>

BEG_NAME_SPACE
struct DX12BufferInfo;
struct AbandoningResource;
using MapOfBufferInfo = std::map<BufferHandle, DX12BufferInfo>;
using VectorOfHeap = std::vector< SmartPTR<ID3D12Heap> >;

using VectorOfAbandoningResources = std::vector< AbandoningResource >;

constexpr static uint32_t BUFFER_SIZE_USED_COMMITTED_RESOURCE = HEAP_CHUNK_SIZE / 2; /**< 创建committed类型资源的起始大小 暂时定为heap管理的堆大小的一半 */
constexpr static uint32_t BUFFER_SIZE_RELEASE_IMMEDIATELY = HEAP_CHUNK_SIZE * 2; /** 假如缓冲区大于等于该值，则一旦不被使用就应该立即释放 暂时定为heap管理的堆大小的两倍 */


class DX12ResourceManager {
	struct BufferInfo {
		SmartPTR<ID3D12Resource> buffer = nullptr;
		DX12HeapManager::BlockInfo info = {}; /**< 占用堆的情况，假如不适用heap manager，则内部数据全是0 */
		size_t size = 0; /**< buffer初次申请的大小 */
		ResourceStatus status = {};
		bool onUpload = false;
		bool isFree = true;
	};
	/** 存储所有创建了的buffer资源，buffer按照申请的大小从小到大排序 */
	struct BufferPool {
		using Pool = std::list<BufferInfo>;
		using iterator = Pool::iterator;
		using const_iterator = Pool::const_iterator;
		Pool buffers;
		BufferPool() = default;
		BufferPool(const BufferPool&) = delete;
		BufferPool(BufferPool&&) = delete;
		BufferPool& operator=(const BufferPool&) = delete;
		BufferPool& operator=(BufferPool&&) = delete;
		inline void swap(BufferPool& pool) { buffers.swap(pool.buffers); }
		/** 顺序查询缓存池，返回首个>= size的buffer的迭代器
		 * 若没有，则返回end */
		inline iterator lower_bound(size_t size) {
			iterator res = buffers.begin();
			for (; res != buffers.end() && res->size < size; ++res) {}
			return res;
		}
		/** 向缓存池插入新的buffer
		 * 确保插入的位置n满足 (n-1).size <= n.size <= (n+1).size */
		inline iterator insert(const BufferInfo& info) {
			iterator pos = lower_bound(info.size); /** 返回 iter >= size */
			if (pos == buffers.end()) {
				buffers.push_back(info);
				return std::prev(buffers.end());
			}
			else return  buffers.insert(pos, info);
		}
		inline iterator insert(BufferInfo&& info) {
			iterator pos = lower_bound(info.size);
			if (pos == buffers.end()) {
				buffers.push_back(std::move(info));
				return std::prev(buffers.end());
			}
			else return buffers.insert(pos, std::move(info));
		}
		inline void erase(iterator pos) { buffers.erase(pos); }
		inline const_iterator end() const { return buffers.end(); }
		inline iterator end() { return buffers.end(); }
	};
public:
	DX12ResourceManager(ID3D12Device* device) : m_device(device) {
		m_uploadHeapMgr = std::make_unique<DX12HeapManager>(device, D3D12_HEAP_TYPE_UPLOAD);
		m_defaultHeapMgr = std::make_unique<DX12HeapManager>(device, D3D12_HEAP_TYPE_DEFAULT);
	}
	~DX12ResourceManager() = default; /**< 假如有资源正在被引用，可能会造成错误 */
	DX12ResourceManager(const DX12ResourceManager&) = delete;
	DX12ResourceManager(DX12ResourceManager&& rhs);
	DX12ResourceManager& operator=(const DX12ResourceManager&) = delete;
	DX12ResourceManager& operator=(DX12ResourceManager&&);
public:
	/** 根据请求返回一个满足要求且可用的buffer资源
	 * @param size 请求的buffer的大小，单位字节
	 * @param usage 请求的buffer的用途
	 * @param flag 请求的buffer的特性
	 * @return 创建成功返回buffer的指针以及当前的初始状态；创建失败返回{NULL, INVALID}
	 * @remark 返回的state可能会有很多种！需要进行一定的状态切换*/
	auto RequestBuffer(size_t size, ResourceUsage usage, ResourceFlag flag)
		-> std::pair<ID3D12Resource*, ResourceState> thread_safe;
	/** 向管理器归还申请的缓冲
	 * @param res 之前请求的缓冲的指针
	 * @param state 之前请求的缓冲当前的状态
	 * @return 归还成功返回true，失败返回false */
	bool RevertBuffer(ID3D12Resource* res, ResourceState state) thread_safe;
	
	void Statistics() {
		char outputInfo[128] = { 0 };
		size_t totalFreeSize = 0;
		size_t totalUsedSize = 0;
		size_t totalUploadHeapSize = 0;
		size_t totalDefaultHeapSize = 0;
		for (auto bufferInfo : m_buffers.buffers) {
			if (bufferInfo.isFree) totalFreeSize += bufferInfo.size;
			else totalUsedSize += bufferInfo.size;
			if (bufferInfo.info.size) {
				if (bufferInfo.onUpload) totalUploadHeapSize += bufferInfo.info.size;
				else totalDefaultHeapSize += bufferInfo.info.size;
			}
		}
		sprintf(outputInfo, "req buffer size is : %zu, free buffer size is : %zu, used buffer size is : %zu \n",
			totalFreeSize + totalUsedSize, totalFreeSize, totalUsedSize);
		MLOG(outputInfo);
		sprintf(outputInfo, "used upload heap size: %zu, used default heap size: %zu \n", totalUploadHeapSize, totalDefaultHeapSize);
		MLOG(outputInfo);
		sprintf(outputInfo, "upload heap: \n");
		MLOG(outputInfo);
		m_uploadHeapMgr->Statistics();
		sprintf(outputInfo, "default heap: \n");
		MLOG(outputInfo);
		m_defaultHeapMgr->Statistics();
	}
	/** 将当前所有申请而未被使用的buffer释放 */
	void flush() thread_safe;
private:
	
	/** 负责进行右值交换 */
	inline void swapWithRValue(DX12ResourceManager&& rhs);
	inline bool placedBufferOnUploadHeap(ResourceFlag flag) thread_safe { return flag == RESOURCE_FLAG_FREQUENTLY; }
	/** 根据请求，创建新buffer资源，并加入到buffer池中
	 * @param size 请求的buffer的大小，单位字节
	 * @param usage 请求的buffer的用途 
	 * @param flag 请求的buffer的特性
	 * @return 创建成功返回buffer的指针以及当前的初始状态；创建失败返回{NULL, INVALID}*/
	auto genNewBuffer(size_t size, ResourceUsage usage, ResourceFlag flag)
		->std::pair<ID3D12Resource*, ResourceState> thread_safe;
	/** 从缓存池中寻找没有被使用的，满足申请要求的buffer资源
	 * @param size 请求的buffer的大小，单位字节
	 * @param usage 请求的buffer的用途
	 * @param flag 请求的buffer的特性
	 * @return 创建成功返回buffer的指针以及当前的初始状态；创建失败返回{NULL, INVALID}*/
	auto findoutFreeBuffer(size_t size, ResourceUsage usage, ResourceFlag flag)
		->std::pair<ID3D12Resource*, ResourceState> thread_safe;
private:
	ID3D12Device* m_device;
	std::mutex m_bufferLock;
	BufferPool m_buffers; /**< 当前已经申请的所有缓冲 */
	std::map<std::uintptr_t, BufferPool::iterator> m_addrIndex; /**< 缓冲地址和缓冲迭代器对 */
	std::unique_ptr<DX12HeapManager> m_uploadHeapMgr;
	std::unique_ptr<DX12HeapManager> m_defaultHeapMgr;
};

END_NAME_SPACE
