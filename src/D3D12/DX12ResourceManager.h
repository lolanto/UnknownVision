#pragma once

#include "DX12Config.h"
#include "DX12HeapManager.h"
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
	struct BufferInfo {
		BufferInfo() = default;
		BufferInfo(SmartPTR<ID3D12Resource>&& buf, const DX12ResourceHeapManager::BlockInfo& block,
			D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType,
			size_t size, bool isFree)
			: buffer(std::move(buf)), block(block), state(state), flags(flags), heapType(heapType), size(size), isFree(isFree) {}
		BufferInfo(const SmartPTR<ID3D12Resource>& buf, const DX12ResourceHeapManager::BlockInfo& block,
			D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType,
			size_t size, bool isFree)
			: buffer(buf), block(block), state(state), flags(flags), heapType(heapType), size(size), isFree(isFree) {}
		BufferInfo(BufferInfo&& buf)
			: buffer(std::move(buf.buffer)), state(buf.state), flags(buf.flags), heapType(buf.heapType), size(buf.size), isFree(buf.isFree), block(buf.block) {}
		BufferInfo(const BufferInfo& buf)
			: buffer(buf.buffer), state(buf.state), flags(buf.flags), heapType(buf.heapType), size(buf.size), isFree(buf.isFree), block(buf.block) {}
		BufferInfo& operator=(BufferInfo&&) = delete;
		BufferInfo& operator=(const BufferInfo&&) = delete;
		/** Properties */
		SmartPTR<ID3D12Resource> buffer = nullptr;
		const DX12ResourceHeapManager::BlockInfo block = {}; /**< 堆占用情况，假如不使用heap manager，则内部数据全是0 */
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
		const D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		const D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
		const size_t size = 0; /**< 初次申请缓冲区的大小，单位字节 */
		bool isFree = true;
	};
	struct TextureInfo {
		TextureInfo() = default;
		TextureInfo(SmartPTR<ID3D12Resource>&& tex, const DX12ResourceHeapManager::BlockInfo& block, D3D12_RESOURCE_STATES state,
			D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT format, uint32_t width, uint32_t height,
			uint16_t mipLevel, bool isFree)
			: texture(std::move(tex)), block(block), state(state), flags(flags),
			elementFormat(format), width(width), height(height), mipLevel(mipLevel), isFree(isFree) {}
		TextureInfo(const SmartPTR<ID3D12Resource>& tex, const DX12ResourceHeapManager::BlockInfo& block, D3D12_RESOURCE_STATES state,
			D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType, DXGI_FORMAT format, uint32_t width, uint32_t height,
			uint16_t mipLevel, bool isFree)
			: texture(tex), block(block), state(state), flags(flags),
			elementFormat(format), width(width), height(height), mipLevel(mipLevel), isFree(isFree) {}
		TextureInfo(TextureInfo&& tex)
			: texture(std::move(tex.texture)), block(tex.block), state(tex.state), flags(tex.flags),
			elementFormat(tex.elementFormat), width(tex.width), height(tex.height), mipLevel(tex.mipLevel), isFree(tex.isFree) {}
		TextureInfo(const TextureInfo& tex)
			: texture(tex.texture), block(tex.block), state(tex.state), flags(tex.flags),
			elementFormat(tex.elementFormat), width(tex.width), height(tex.height), mipLevel(tex.mipLevel), isFree(tex.isFree) {}
		TextureInfo& operator=(TextureInfo&&) = delete;
		TextureInfo& operator=(const TextureInfo&) = delete;
		/** Properties */
		SmartPTR<ID3D12Resource> texture = nullptr;
		const DX12ResourceHeapManager::BlockInfo block = {}; /**< 堆占用情况，假如不使用heap manager，则内部数据全是0 */
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
		const D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		const DXGI_FORMAT elementFormat = DXGI_FORMAT_UNKNOWN; /**< 申请时的纹理元素的格式 */
		const uint32_t width = 0; /**< 初次申请纹理时的宽度，最多不超过4G个，单位像素 */
		const uint32_t height = 0; /**< 初次申请纹理时的高度，最多不超过4G个，单位像素 */
		const uint16_t mipLevel = 0; /**< 初次申请时的细分等级 */
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
	struct TexturePool {
		using Pool = std::list<TextureInfo>;
		using iterator = Pool::iterator;
		using const_iterator = Pool::const_iterator;
		Pool textures;
		TexturePool() = default;
		TexturePool(const TexturePool&) = delete;
		TexturePool(TexturePool&&) = delete;
		TexturePool& operator=(const TexturePool&) = delete;
		TexturePool& operator=(TexturePool&&) = delete;
		inline void swap(TexturePool& pool) { textures.swap(pool.textures); }
		/** 顺序查询纹理池，返回首个>= width && >= height的buffer的迭代器
		 * 若没有，则返回end */
		inline iterator lower_bound(uint32_t width, uint32_t height) {
			iterator res = textures.begin();
			for (; res != textures.end(); ++res) {
				if (res->width >= width && res->height >= height) break;
			}
			return res;
		}
		inline iterator lower_bound_width(uint32_t width) {
			iterator res = textures.begin();
			for (; res != textures.end(); ++res) {
				if (res->width >= width) break;
			}
			return res;
		}
		/** 向纹理池插入新的texture
		 * 确保插入的位置n满足(n-1).width <= width && width <= (n+1).width
		 * (n-1).height <= height && height <= (n+1).height */
		inline iterator insert(const TextureInfo& info) {
			iterator pos = lower_bound_width(info.width); /** 返回 pos.width >= width */
			while (pos != textures.end() && pos->width == info.width) {
				if (pos->height >= info.height) break;
			}
			if (pos == textures.end()) {
				textures.push_back(info);
				return std::prev(pos);
			}
			return textures.insert(pos, info);
		}
		inline iterator insert(TextureInfo&& info) {
			iterator pos = lower_bound_width(info.width); /** 返回 pos.width >= width */
			while (pos != textures.end() && pos->width == info.width) {
				if (pos->height >= info.height) break;
			}
			if (pos == textures.end()) {
				textures.push_back(std::move(info));
				return std::prev(pos);
			}
			return textures.insert(pos, std::move(info));
		}
		inline void erase(iterator pos) { textures.erase(pos); }
		inline const_iterator end() const { return textures.end(); }
		inline iterator end() { return textures.end(); }
	};
public:
	DX12ResourceManager(ID3D12Device* device) : m_device(device) {
		m_uploadHeapMgr = std::make_unique<DX12ResourceHeapManager>(device, D3D12_HEAP_TYPE_UPLOAD);
		m_defaultHeapMgr = std::make_unique<DX12ResourceHeapManager>(device, D3D12_HEAP_TYPE_DEFAULT);
	}
	~DX12ResourceManager() = default; /**< 假如有资源正在被引用，可能会造成错误 */
	DX12ResourceManager(const DX12ResourceManager&) = delete;
	DX12ResourceManager(DX12ResourceManager&& rhs);
	DX12ResourceManager& operator=(const DX12ResourceManager&) = delete;
	DX12ResourceManager& operator=(DX12ResourceManager&&);
public:
	/** 根据请求返回一个满足要求且可用的buffer资源
	 * @param size 请求的buffer的大小，单位字节
	 * @param flag 请求的buffer的特性(用途)
	 * @param heapType 请求的buffer所在的堆类型
	 * @return 创建成功返回buffer的指针以及当前的初始状态；创建失败返回{NULL, D3D12_RESOURCE_STATE_COMMON}
	 * @remark 返回的state可能会有很多种！需要进行一定的状态切换*/
	auto RequestBuffer(size_t size, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType)
		-> std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> thread_safe;
	/** 向管理器归还申请的缓冲
	 * @param res 之前请求的缓冲的指针
	 * @param state 之前请求的缓冲当前的状态
	 * @return 归还成功返回true，失败返回false */
	bool RevertBuffer(ID3D12Resource* res, D3D12_RESOURCE_STATES state) thread_safe;
	/** 向管理器请求一个满足要求，且可用的纹理资源
	 * @param width, height 请求的纹理的长宽，单位像素
	 * @param elementFormat 纹理像素的格式
	 * @param flags 纹理资源的特性(用途)
	 * @mipLevel 纹理资源的细分等级，0表示创建长宽可创建的细分上限 
	 * @return 创建成功返回纹理指针以及资源当前的状态，创建失败返回nullptr以及D3D12_RESOURCE_STATE_COMMON
	 * @remark 返回的state可能有多种可能，需要进行状态切换
	 * 不再提供heapType设置，因为texture只允许创建在default类型的heap上 */
	auto RequestTexture(uint32_t width, uint32_t height, DXGI_FORMAT elementFormat,
		D3D12_RESOURCE_FLAGS flags,
		uint16_t mipLevel) -> std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> thread_safe;

	/** 向管理器归还申请的纹理
	 * @param res 之前请求的纹理的指针
	 * @param state 之前请求的纹理当前的状态
	 * @return 归还成功返回true, 失败返回false */
	bool RevertTexture(ID3D12Resource* res, D3D12_RESOURCE_STATES state) thread_safe;

	/** 输出当前管理器状态 @remark: 非线程安全! */
	void Statistics();

	/** 将当前所有申请而未被使用的资源释放 */
	void flush() thread_safe;
private:
	
	/** 负责进行右值交换 */
	inline void swapWithRValue(DX12ResourceManager&& rhs);
	/** 根据请求，创建新buffer资源，并加入到buffer池中
	 * @param size 请求的buffer的大小，单位字节
	 * @param flag 请求的buffer的特性(用途)
	 * @param heapType 纹理资源所存放的堆
	 * @return 创建成功返回buffer的指针以及当前的初始状态；创建失败返回{NULL, D3D12_RESOURCE_STATE_COMMON}*/
	auto genNewBuffer(size_t size, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType)
		->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> thread_safe;
	/** 从缓存池中寻找没有被使用的，满足申请要求的buffer资源
	 * @param size 请求的buffer的大小，单位字节
	 * @param flag 请求的buffer的特性(用途)
	 * @param heapType 请求的buffer所在的堆类型
	 * @return 创建成功返回buffer的指针以及当前的初始状态；创建失败返回{NULL, D3D12_RESOURCE_STATE_COMMON}*/
	auto findoutFreeBuffer(size_t size, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType)
		->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> thread_safe;

	/** 请求一个满足要求，且可用的纹理资源
	 * @param width, height 请求的纹理的长宽，单位像素
	 * @param elementFormat 纹理像素的格式
	 * @param flags 纹理资源的特性(用途)
	 * @mipLevel 纹理资源的细分等级，0表示创建长宽可创建的细分上限
	 * @return 创建成功返回纹理指针以及资源当前的状态，创建失败返回nullptr以及D3D12_RESOURCE_STATE_COMMON
	 * 不再提供heapType设置，因为texture只允许创建在default类型的heap上 */
	auto genNewTexture(uint32_t width, uint32_t height, DXGI_FORMAT elementFormat,
		D3D12_RESOURCE_FLAGS flags,
		uint16_t mipLevel)
		->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> thread_safe;
	/** 从纹理池中寻找没有被使用的，满足申请要求的纹理资源
	 * @param width, height 请求的纹理的长宽，单位像素
	 * @param elementFormat 纹理像素的格式
	 * @param flags 纹理资源的特性(用途)
	 * @mipLevel 纹理资源的细分等级，0表示创建长宽可创建的细分上限
	 * @return 创建成功返回纹理指针以及资源当前的状态，创建失败返回nullptr以及D3D12_RESOURCE_STATE_COMMON
	 * 不再提供heapType设置，因为texture只允许创建在default类型的heap上 */
	auto findoutFreeTexture(uint32_t width, uint32_t height, DXGI_FORMAT elementFormat,
		D3D12_RESOURCE_FLAGS flags,
		uint16_t mipLevel)
		->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> thread_safe;
private:
	ID3D12Device* m_device;
	std::mutex m_bufferLock;
	BufferPool m_buffers; /**< 当前已经申请的所有缓冲 */
	std::map<std::uintptr_t, BufferPool::iterator> m_bufferAddrIndex; /**< 缓冲地址和缓冲迭代器对 */
	std::mutex m_textureLock;
	TexturePool m_textures; /**< 当前已经申请的所有纹理 */
	std::map<std::uintptr_t, TexturePool::iterator> m_textureAddrIndex; /**< 纹理地址和纹理迭代器对 */
	std::unique_ptr<DX12ResourceHeapManager> m_uploadHeapMgr;
	std::unique_ptr<DX12ResourceHeapManager> m_defaultHeapMgr;
};

END_NAME_SPACE
