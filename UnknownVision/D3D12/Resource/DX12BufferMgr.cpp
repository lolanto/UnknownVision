#include "DX12ResMgr.h"
#include "../DX12RenderSys.h"
#include "../../UVRoot.h"

namespace UnknownVision {
	BufferIdx DX12_BufferMgr::CreateVertexBuffer(size_t numVtxs, size_t vtxSize, uint8_t* data, BufferFlagCombination flag) {
		if (flag & BF_INVALID) return BufferIdx(-1);
		if (!(flag & (BF_WRITE_BY_CPU | BF_WRITE_BY_GPU)) && data == nullptr) {
			MLOG(LL, __FUNCTION__, LW, " create buffer failed, there's no initial data while buffer can't be read any more");
			return BufferIdx(-1);
		}
		ID3D12Device5* dev = static_cast<DX12_RenderSys&>(Root::GetInstance().GetRenderSys()).GetDevice();
		assert(dev != nullptr);

		/** 暂时将顶点缓存以Commited Resource的形式存储 */

		D3D12_HEAP_PROPERTIES heapProp; /**< 描述存储该缓冲的堆的属性 */
		heapProp.Type = (flag & BF_WRITE_BY_CPU) ? 
			D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD; /**< 假如CPU可以写，则使用Default类型的堆；否则使用upload的 */
		/** 抽象堆类型必须的默认值 */
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		/** 暂时不考虑多设备 */
		heapProp.CreationNodeMask = 0;
		heapProp.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC resourceDesc; /** 资源描述结构 */
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0; /**< 缓存类型的资源，会被设置为64KB */
		resourceDesc.Width = numVtxs * vtxSize; /**< 缓冲区的字节大小 */
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE; /**< Flag更多是对纹理类型的资源进行设置 */
		resourceDesc.MipLevels = 1; /**< 缓冲区不使用mip */
		resourceDesc.DepthOrArraySize = 1;
		/** buffer类型资源必须的默认值 */
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.Height = 1;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (heapProp.Type == D3D12_HEAP_TYPE_UPLOAD) resourceState |= D3D12_RESOURCE_STATE_GENERIC_READ;

		SmartPTR<ID3D12Resource1> res;
		if (FAILED(dev->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
			&resourceDesc, resourceState, nullptr, IID_PPV_ARGS(&res)))) {
			MLOG(LL, __FUNCTION__, LW, "create vertex buffer failed!");
			return BufferIdx(-1);
		}
		/** 假如有初始数据，进行初始化 */
		if (data) {
			void* pData = nullptr;
			res->Map(0, nullptr, &pData); /**< map所有内容 */
			memcpy(pData, reinterpret_cast<void*>(data), numVtxs * vtxSize);
			res->Unmap(0, nullptr);
		}

		m_buffers.push_back(DX12_Buffer(res, numVtxs * vtxSize, numVtxs, BT_VERTEX_BUFFER, flag, m_buffers.size()));
		return BufferIdx(m_buffers.size() - 1);
	}
}