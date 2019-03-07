#ifndef UV_D3D12_RESOURCE_MANAGER_H
#define UV_D3D12_RESOURCE_MANAGER_H

#include "../DX12Config.h"
#include "../../ResourceManager/BufferManager.h"
#include "../Resource/DX12Buffer.h"
#include <vector>

namespace UnknownVision {
	class DX12_BufferMgr : public BufferMgr {
	public:
		DX12_BufferMgr() = default;
		~DX12_BufferMgr() = default;
	public:
		/** 创建顶点缓冲
		 * @param numVtx 缓冲中顶点的数量，设置后不能修改
		 * @param vtxSize 缓冲中一个顶点的字节大小
		 * @param data 顶点缓冲的初始数据
		 * @param flags 缓冲区的特性，详细内容见BufferFlag定义
		 * @return 创建成功，返回缓冲区的索引；创建失败返回-1 */
		virtual BufferIdx CreateVertexBuffer(size_t numVtxs, size_t vtxSize, uint8_t* data, BufferFlagCombination flags);
	private:
		std::vector<DX12_Buffer> m_buffers;
	};
}

#endif // UV_D3D12_RESOURCE_MANAGER_H
