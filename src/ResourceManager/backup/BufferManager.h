#ifndef UV_BUFFER_MANAGER_H
#define UV_BUFFER_MANAGER_H

#include "ResourceManangerConfig.h"

namespace UnknownVision {
	class BufferMgr : public ResourceMgr {
	public:
		BufferMgr() : ResourceMgr(MT_BUFFER_MANAGER) {}
		virtual ~BufferMgr() {}
	public:
		/** 创建顶点缓冲
		 * @param numVtx 缓冲中顶点的数量，设置后不能修改
		 * @param vtxSize 缓冲中一个顶点的字节大小
		 * @param data 顶点缓冲的初始数据
		 * @param flags 缓冲区的特性，详细内容见BufferFlag定义
		 * @return 创建成功，返回缓冲区的索引；创建失败返回-1 */
		virtual BufferIdx CreateVertexBuffer(size_t numVtxs, size_t vtxSize, uint8_t* data, BufferFlagCombination flags) = 0;
		/** 创建常量缓冲区
		 * @param byteSize 常量缓冲区的大小
		 * @param data 用于初始化常量缓冲区的数据
		 * @param flags 缓冲区的额外设置
		 * @return 创建成功，返回索引；否则返回-1 */
		virtual BufferIdx CreateConstantBuffer(size_t byteSize, uint8_t* data, BufferFlagCombination flags) = 0;
		/** 创建索引缓存
		 * @param numIdxs 缓冲中索引的数量
		 * @param IdxSize 一个索引元素的大小
		 * @param data 用于初始化索引缓冲区的数据
		 * @param flags 缓冲区的额外设置
		 * @return 创建成功返回缓冲索引，否则返回-1 */
		virtual BufferIdx CreateIndexBuffer(size_t numIdxs, size_t IdxSize, uint8_t* data, BufferFlagCombination flags) = 0;
		virtual Buffer& GetBuffer(BufferIdx index) = 0;
	};


}

#endif // UV_BUFFER_MANAGER_H
