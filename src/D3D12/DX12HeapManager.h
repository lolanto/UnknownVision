#pragma once

#include "../Utility/InfoLog/InfoLog.h"
#include "DX12Config.h"
#include <mutex>
#include <map>
#include <cassert>
#include <iostream>
#include <limits>

BEG_NAME_SPACE
using HeapBlockOffset = size_t;

using HeapBlockSize = size_t;
using HeapBlockAlignment = uint32_t;

constexpr static uint32_t HEAP_BLOCK_LIMIT = UINT32_MAX; /**< 一次申请的堆的大小上限，暂时是4G */
constexpr static uint32_t HEAP_CHUNK_SIZE = 1ul << 26; /**< 一个堆大小64M */
constexpr static uint32_t MAX_NUMBER_OF_HEAP = 4; /**< 一个堆管理器最多有4个堆 */
constexpr static size_t MAX_STORAGE_OF_HEAP_MANAGER = /**< 一个堆管理器最大的存储空间 */
(size_t)HEAP_CHUNK_SIZE * (size_t)MAX_NUMBER_OF_HEAP;
constexpr static uint32_t HEAP_DEFAULT_ALIGNMENT = 1 << 22; /**< 每个堆的默认对齐值(取DX12的最大对齐值) */
constexpr static uint32_t BLOCK_SMALLEST_ALIGNMENT = 1 << 12; /**< 每个block的最小对齐值 4K */

static_assert((std::numeric_limits<HeapBlockSize>::max)() >= HEAP_BLOCK_LIMIT);
static_assert((std::numeric_limits<HeapBlockOffset>::max)() >= MAX_STORAGE_OF_HEAP_MANAGER);

class BasicHeapManager {
public:
	virtual ~BasicHeapManager() = default;
	BasicHeapManager() = default;
public:
	struct BlockInfo {
		HeapBlockOffset offset = 0;
		HeapBlockSize size = 0;
	};
public:
	/** 向管理器请求堆空间
 * @param size 请求的堆空间大小
 * @param alignment 空间的对齐要求
 * @param chunkSize 一个chunk的大小，也是一个block可以申请的最大大小
 * @param minimumBlockSize 最小剩余块大小，分配完成后的余下块大小必须大于该值
 * @return 返回分配的块的offset以及size，假如分配失败块大小为0
 * @invariance 多个请求时必须保证多个请求都有唯一分配的空间， 或者分配失败*/
	BlockInfo RequestBlock(HeapBlockSize size, HeapBlockAlignment alignment, HeapBlockSize chunkSize, HeapBlockSize minimumBlockSize) thread_safe {
		BlockInfo newBlock = {};
		do {
			std::lock_guard<std::mutex> lg(m_freeBlockMutex);
			auto blockIter = m_freeBlocksInSize.lower_bound(size);
			while (blockIter != m_freeBlocksInSize.end()) {
				if (blockIter->second % alignment == 0) { /**< 满足对齐要求 */
					BlockInfo restBlock = {}; /**< 剩余块的信息 */
					/** 可以使用该block */
					/** 假如即将要分配的block在分配完size后剩余的空间不足最小对齐值
					 * 则返回整块block，否则将block分裂成两块，一块用于分配另外一块保留 */
					newBlock.size = blockIter->first - size >= minimumBlockSize ? size : blockIter->first;
					newBlock.offset = blockIter->second;
					restBlock.size = blockIter->first - newBlock.size;
					restBlock.offset = newBlock.offset + newBlock.size;

					/** 删除原有的block并添加新的free block信息 */
					/** 1. 删除原有的blockInOffset，增加新的blockInOffset */
					m_freeBlocksInOffset.erase(blockIter->second);
					if (restBlock.size != 0) m_freeBlocksInOffset.insert(std::make_pair(restBlock.offset, restBlock.size));
					m_freeBlocksInSize.erase(blockIter);
					if (restBlock.size != 0) m_freeBlocksInSize.insert(std::make_pair(restBlock.size, restBlock.offset));
					return newBlock;
				}
				else {
					++blockIter;
				}
			}
			/** 目前的free block中都不能满足该请求，申请创建新的heap */
		} while (createHeap());
		/** 假如不能再创建heap，则返回失败的分配 */
		return newBlock;
	}

	/** 向管理器归还空间
	 * @param blockInfo 需要归还的空间的信息
	 * @param chunkSize 一个chunk的大小，也是一个heapBlock最大的大小
	 * @invariance 多个请求时保证空间管理信息的正确性 */
	void RevertBlock(BlockInfo blockInfo, HeapBlockSize chunkSize) thread_safe {
		/** 更新两个freeBlocks变量，并保证这两个变量的一致性 */
		std::lock_guard<std::mutex> lg(m_freeBlockMutex);
		/** 可以获得邻近的两个block的迭代器，这是由map的标准保证的——迭代器的+/-操作可以访问邻近的key/value */
		auto nextBlock = m_freeBlocksInOffset.upper_bound(blockInfo.offset);
		auto prevBlock = m_freeBlocksInOffset.end();
		/** 判断nextBlock不处在开头，nextBlock前还有block */
		if (!m_freeBlocksInOffset.empty() && nextBlock != m_freeBlocksInOffset.begin())
			prevBlock = std::prev(nextBlock);
		BlockInfo newBlock = blockInfo;
		/** 先前融合，前提是prevBlock存在
		 * 同时freeBlock不是处在所属堆的开头(offset % HEAP_CHUNK_SIZE != 0) */
		if (prevBlock != m_freeBlocksInOffset.end()) { /**< prevBlock存在 */
			if (blockInfo.offset % chunkSize /**< freeBlock不是在开头 */
				&& prevBlock->first + prevBlock->second == blockInfo.offset) {
				/** 可以融合，因为两个block相邻 */
				newBlock.offset = prevBlock->first;
				newBlock.size += prevBlock->second;
			}
			else {
				prevBlock = m_freeBlocksInOffset.end();
			}
		}
		/** 向后融合，前提是freeBlock之后还有freeBlock
		 * 同时freeBlock的末尾不在堆的边界*/
		if (nextBlock != m_freeBlocksInOffset.end()) {
			if (nextBlock->first % chunkSize
				&& blockInfo.offset + blockInfo.size == nextBlock->first) {
				/** 可以融合，因为两个block相邻 */
				newBlock.size += nextBlock->second;
			}
			else {
				nextBlock = m_freeBlocksInOffset.end();
			}
		}
		/** 保证freeBlocksInSize的一致性 */
		{
			decltype(m_freeBlocksInOffset)& inOffset = m_freeBlocksInOffset;
			decltype(m_freeBlocksInSize)& inSize = m_freeBlocksInSize;
			/** 该临时函数输入需要删除的block在Offset中的迭代器，并将该block从Offset和Size中删除 */
			auto funcRemoveMergedBlocks = [&inOffset, &inSize](decltype(m_freeBlocksInOffset)::iterator& offsetIterator) {
				if (offsetIterator != inOffset.end()) {
					auto range = inSize.equal_range(offsetIterator->second); /** 先挑选出所有与待删除block拥有相同size的block */
					for (auto sizeIterator = range.first; sizeIterator != range.second; ++sizeIterator) {
						if (sizeIterator->second == offsetIterator->first) {  /**< "offset" inSize == "offset" inOffset */
							inSize.erase(sizeIterator);
							break;
						}
					}
					inOffset.erase(offsetIterator);
				}
			};
			funcRemoveMergedBlocks(prevBlock);
			funcRemoveMergedBlocks(nextBlock);
		}

		/** 插入新的block */
		m_freeBlocksInSize.insert(std::make_pair(newBlock.size, newBlock.offset));
		m_freeBlocksInOffset.insert(std::make_pair(newBlock.offset, newBlock.size));
	}
	virtual void Reset() thread_safe{
		std::lock_guard<std::mutex> flg(m_freeBlockMutex);
		m_freeBlocksInOffset.clear();
		m_freeBlocksInSize.clear();
	}
protected:
	virtual bool createHeap() = 0 thread_safe;
protected:
	/** 以下两个freeBlocks变量的一致性：两者所能查询的信息必须时刻相同 */
	std::mutex m_freeBlockMutex; /**< 维护以下两个变量的锁 */
	std::multimap<HeapBlockSize, HeapBlockOffset> m_freeBlocksInSize;
	std::map<HeapBlockOffset, HeapBlockSize> m_freeBlocksInOffset;
};

class DX12ResourceHeapManager : public BasicHeapManager {
public:
	const D3D12_HEAP_TYPE Type;
	DX12ResourceHeapManager(ID3D12Device* device, D3D12_HEAP_TYPE type) : m_device(device), Type(type) {}
	/** 向管理器请求堆空间
	 * @param size 请求的堆空间大小
	 * @param alignment 空间的对齐要求，必须是2的幂
	 * @return heap, offset, 假如分配失败 heap = nullptr
	 * @invariance 多个请求时必须保证多个请求都有唯一分配的空间， 或者分配失败*/
	auto RequestBlock(HeapBlockSize size, HeapBlockAlignment alignment)
		->std::tuple<ID3D12Heap*, BlockInfo> thread_safe {
		assert(alignment % 2 == 0 && size > 0 && size < HEAP_CHUNK_SIZE);
		BlockInfo block = BasicHeapManager::RequestBlock(size, alignment, HEAP_CHUNK_SIZE, BLOCK_SMALLEST_ALIGNMENT);
		if (block.size == 0) return { nullptr, block };
		ID3D12Heap* heap = GetHeap(block.offset);
		return { heap, block };
	}

	/** 向管理器归还空间
	 * @param blockInfo 需要归还的空间的信息
	 * @invariance 多个请求时保证空间管理信息的正确性 */
	void RevertBlock(BlockInfo blockInfo) thread_safe {
		BasicHeapManager::RevertBlock(blockInfo, HEAP_CHUNK_SIZE);
	}
	ID3D12Heap* GetHeap(HeapBlockOffset offset) thread_safe {
		return m_heaps[offset / HEAP_CHUNK_SIZE].Get();
	}

	auto HeapSize() const {
		return m_heaps.size() * HEAP_CHUNK_SIZE;
	}

	/**< 输出当前heap manager 的状态信息 */
	void Statistics() {
		char outputInfo[128] = { 0 };
		size_t totalFreeSize = 0;
		for (auto freeBlock : m_freeBlocksInSize) { totalFreeSize += freeBlock.first; }
		sprintf(outputInfo, "Current number of heaps is %zu, total free size is %zu \n", m_heaps.size(), totalFreeSize);
		MLOG(outputInfo);
	}

	void Reset() final {
		{
			std::lock_guard<std::mutex> hlg(m_heapMutex);
			m_heaps.swap(decltype(m_heaps)());
		}
		BasicHeapManager::Reset();
	}
private:
	bool createHeap() final thread_safe {
		size_t newOffset = 0;
		{
			/** 使用try lock是为了避免多个线程请求创建heap而造成创建了多个heap的情况
			* 只允许一个线程创建heap，剩下的请求都会驳回，并不断尝试重新创建新的block */
			std::unique_lock<std::mutex> ul(m_heapMutex, std::try_to_lock);
			if (!ul) return true;
			if (m_heaps.size() == MAX_NUMBER_OF_HEAP) return false;
			Microsoft::WRL::ComPtr<ID3D12Heap> newHeap;
			D3D12_HEAP_DESC desc;
			desc.Alignment = HEAP_DEFAULT_ALIGNMENT;
			desc.Flags = D3D12_HEAP_FLAG_NONE;
			desc.Properties.VisibleNodeMask = 0;
			desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			desc.Properties.CreationNodeMask = 0;
			desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			desc.Properties.Type = Type;
			desc.SizeInBytes = HEAP_CHUNK_SIZE;
			if (FAILED(m_device->CreateHeap(&desc, IID_PPV_ARGS(&newHeap))))
				return false;
			/** 压入新的freeblock */
			std::lock_guard<std::mutex> lg(m_freeBlockMutex);
			m_heaps.push_back(newHeap);
			newOffset = (m_heaps.size() - 1) * HEAP_CHUNK_SIZE;
			m_freeBlocksInOffset.insert(std::make_pair(newOffset, HEAP_CHUNK_SIZE));
			m_freeBlocksInSize.insert(std::make_pair(HEAP_CHUNK_SIZE, newOffset));
		}
		return true;
	}
private:
	std::mutex m_heapMutex; /**< 维护heap的锁 */
	std::vector< Microsoft::WRL::ComPtr<ID3D12Heap> > m_heaps;
	ID3D12Device* m_device;
};

constexpr uint8_t DESCRIPTOR_HEAP_DEFAULT_SIZE = 64; /**< 一个descriptor heap能够容纳的最大descriptor */
constexpr uint8_t MAX_NUMBER_OF_DESCRIPTOR_HEAP = 8; /**< 一个manager可以容纳的descriptor heap的数量 */

//class DX12DescriptorHeapManager : public BasicHeapManager {
//public:
//	DX12DescriptorHeapManager() : m_device(nullptr) {}
//	void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12Device* dev) { m_type = type; m_device = dev; }
//	/** 向管理器请求堆空间
//	 * @param size 请求的堆空间大小
//	 * @return heap, offset, 假如分配失败 heap = nullptr
//	 * @invariance 多个请求时必须保证多个请求都有唯一分配的空间， 或者分配失败*/
//	auto RequestBlock(HeapBlockSize size)
//		->std::pair<ID3D12DescriptorHeap*, BlockInfo> thread_safe {
//		assert(size > 0 && size < DESCRIPTOR_HEAP_DEFAULT_SIZE);
//		BlockInfo block = BasicHeapManager::RequestBlock(size, 1, DESCRIPTOR_HEAP_DEFAULT_SIZE, 1);
//		if (block.size == 0) return { nullptr, block };
//		ID3D12DescriptorHeap* heap = GetHeap(block.offset);
//		return { heap, block };
//	}
//
//	/** 向管理器归还空间
//	 * @param blockInfo 需要归还的空间的信息
//	 * @invariance 多个请求时保证空间管理信息的正确性 */
//	void RevertBlock(BlockInfo blockInfo) thread_safe {
//		BasicHeapManager::RevertBlock(blockInfo, DESCRIPTOR_HEAP_DEFAULT_SIZE);
//	}
//
//	ID3D12DescriptorHeap* GetHeap(HeapBlockOffset offset) thread_safe {
//		return m_heaps[offset / DESCRIPTOR_HEAP_DEFAULT_SIZE].Get();
//	}
//
//	auto HeapSize() const {
//		return m_heaps.size() * DESCRIPTOR_HEAP_DEFAULT_SIZE;
//	}
//
//	void Reset() final {
//		{
//			std::lock_guard<std::mutex> hlg(m_heapMutex);
//			m_heaps.swap(decltype(m_heaps)());
//		}
//		BasicHeapManager::Reset();
//	}
//private:
//	bool createHeap() final thread_safe {
//		size_t newOffset = 0;
//		{
//			/** 使用try lock是为了避免多个线程请求创建heap而造成创建了多个heap的情况
//			* 只允许一个线程创建heap，剩下的请求都会驳回，并不断尝试重新创建新的block */
//			std::unique_lock<std::mutex> ul(m_heapMutex, std::try_to_lock);
//			if (!ul) return true;
//			if (m_heaps.size() == MAX_NUMBER_OF_DESCRIPTOR_HEAP) return false;
//			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> newHeap;
//			D3D12_DESCRIPTOR_HEAP_DESC desc;
//			desc.Type = m_type;
//			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//			desc.NodeMask = 0;
//			desc.NumDescriptors = DESCRIPTOR_HEAP_DEFAULT_SIZE;
//			if (FAILED(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newHeap))))
//				return false;
//			/** 压入新的freeblock */
//			std::lock_guard<std::mutex> lg(m_freeBlockMutex);
//			m_heaps.push_back(newHeap);
//			newOffset = (m_heaps.size() - 1) * DESCRIPTOR_HEAP_DEFAULT_SIZE;
//			m_freeBlocksInOffset.insert(std::make_pair(newOffset, DESCRIPTOR_HEAP_DEFAULT_SIZE));
//			m_freeBlocksInSize.insert(std::make_pair(DESCRIPTOR_HEAP_DEFAULT_SIZE, newOffset));
//		}
//		return true;
//	}
//private:
//	D3D12_DESCRIPTOR_HEAP_TYPE m_type;
//
//	std::mutex m_heapMutex; /**< 维护heap的锁 */
//	std::vector< Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> > m_heaps;
//	ID3D12Device* m_device;
//};

END_NAME_SPACE
