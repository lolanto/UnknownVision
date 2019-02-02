#include "DX11ResMgr.h"
#include "../../UVRoot.h"
#include "../DX11RenderSys.h"

namespace UnknownVision {
	/// 快速填充缓冲区描述结构
	/** 根据缓冲区的大小以及缓冲区的flags，将创建
	 *	缓冲区描述的通用代码提取出来集中管理，主要管理
	 *	缓冲区的大小，CPU访问方式以及缓存内存使用方式
	 *	@param byteSize
	 *		缓冲区的大小
	 *	@param flags
	 *		缓冲区的特性组合
	 *	@ret
	 *		返回部分完整的描述结构
	 */
	inline D3D11_BUFFER_DESC createBufferDesc(size_t byteSize, BufferFlagCombination flags) {
		D3D11_BUFFER_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.ByteWidth = byteSize;
		desc.CPUAccessFlags = 0;
		if (flags & BufferFlag::BF_READ_BY_CPU) desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
		if (flags & BufferFlag::BF_WRITE_BY_CPU) desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
		desc.Usage = D3D11_USAGE_DEFAULT;
		/// GPU只读
		if (!(flags & ~BufferFlag::BF_READ_BY_GPU) && (flags & BufferFlag::BF_READ_BY_GPU))
			desc.Usage = D3D11_USAGE_IMMUTABLE;
		/// CPU只写，GPU只读
		else if (flags & BufferFlag::BF_READ_BY_GPU && flags & BufferFlag::BF_WRITE_BY_CPU)
			desc.Usage = D3D11_USAGE_DYNAMIC;
		/// 暂时不考虑D3D11_USAGE_STAGING
		return desc;
	}
	/** 提取创建buffer需要的数据描述对象的创建代码
	 * @param data
	 *		用于指定SubResource包含的缓冲区数据
	 */
	inline D3D11_SUBRESOURCE_DATA createSubResourceDescForBuffer(uint8_t* data) {
		D3D11_SUBRESOURCE_DATA sd;
		sd.pSysMem = data; 
		sd.SysMemPitch = sd.SysMemSlicePitch = 0; // 这两个参数对缓冲区没有意义
		return sd;
	}
	BufferIdx DX11_BufferMgr::CreateVertexBuffer(size_t numVtxs, size_t vtxSize, 
		uint8_t* data, BufferFlagCombination flags) {
		if (flags == BufferFlag::BF_INVALID) return BufferIdx(-1);
		if (!(flags & (BufferFlag::BF_WRITE_BY_CPU | BufferFlag::BF_WRITE_BY_GPU)) 
			&& data == nullptr) {
			MLOG(LE, __FUNCTION__, LL, " can't create vertex buffer which can't be writed any more and has no initail data");
			return BufferIdx(-1);
		}
		ID3D11Device* dev = static_cast<DX11_RenderSys&>(Root::GetInstance().GetRenderSys()).GetDevice();
		assert(dev != nullptr);

		SmartPTR<ID3D11Buffer> bufPtr;
		D3D11_BUFFER_DESC&& bufDesc = createBufferDesc(numVtxs * vtxSize, flags);
		D3D11_SUBRESOURCE_DATA&& subData = createSubResourceDescForBuffer(data);

		bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufDesc.MiscFlags = 0; // 暂时不使用特别的设置
		bufDesc.StructureByteStride = vtxSize;

		if (FAILED(dev->CreateBuffer(&bufDesc, &subData, bufPtr.GetAddressOf()))) {
			MLOG(LE, __FUNCTION__, LL, " create vertex buffer failed!");
			return BufferIdx(-1);
		}

		m_buffers.push_back(DX11_Buffer(numVtxs * vtxSize, numVtxs, 
			BT_VERTEX_BUFFER, flags, bufPtr, m_buffers.size()));

		return BufferIdx(m_buffers.size() - 1);
	}

	BufferIdx DX11_BufferMgr::CreateConstantBuffer(size_t byteSize, uint8_t* data, BufferFlagCombination flags) {
		if (flags == BufferFlag::BF_INVALID) return BufferIdx(-1);
		if (!(flags & (BufferFlag::BF_WRITE_BY_CPU | BufferFlag::BF_WRITE_BY_GPU))
			&& data == nullptr) {
			MLOG(LE, __FUNCTION__, LL, " can't create constant buffer which can't be writed any more and has no initail data");
			return BufferIdx(-1);
		}
		ID3D11Device* dev = static_cast<DX11_RenderSys&>(Root::GetInstance().GetRenderSys()).GetDevice();
		assert(dev != nullptr);

		SmartPTR<ID3D11Buffer> bufPtr;
		D3D11_BUFFER_DESC&& bufDesc = createBufferDesc(byteSize, flags);
		D3D11_SUBRESOURCE_DATA&& subData = createSubResourceDescForBuffer(data);

		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.MiscFlags = 0; // 暂时不做特别处理
		bufDesc.StructureByteStride = 0; // 常量缓冲区暂时不管
		
		if (FAILED(dev->CreateBuffer(&bufDesc, &subData, bufPtr.GetAddressOf()))) {
			MLOG(LE, __FUNCTION__, LL, " create constant buffer failed!");
			return BufferIdx(-1);
		}

		m_buffers.push_back(DX11_Buffer(byteSize, 1, BT_CONSTANT_BUFFER,
			flags, bufPtr, m_buffers.size()));
		return BufferIdx(m_buffers.size() - 1);
	}

	BufferIdx DX11_BufferMgr::CreateIndexBuffer(size_t numIdxs, size_t IdxSize, uint8_t* data, BufferFlagCombination flags) {
		if (flags == BufferFlag::BF_INVALID) return BufferIdx(-1);
		if (!(flags & (BufferFlag::BF_WRITE_BY_CPU | BufferFlag::BF_WRITE_BY_GPU))
			&& data == nullptr) {
			MLOG(LE, __FUNCTION__, LL, " can't create index buffer which can't be writed any more and has no initail data");
			return BufferIdx(-1);
		}
		ID3D11Device* dev = static_cast<DX11_RenderSys&>(Root::GetInstance().GetRenderSys()).GetDevice();
		assert(dev != nullptr);

		SmartPTR<ID3D11Buffer> bufPtr;
		D3D11_BUFFER_DESC&& bufDesc = createBufferDesc(numIdxs * IdxSize, flags);
		D3D11_SUBRESOURCE_DATA&& subData = createSubResourceDescForBuffer(data);

		bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufDesc.MiscFlags = 0; // 暂时不需要进行特殊处理
		bufDesc.StructureByteStride = IdxSize;

		if (FAILED(dev->CreateBuffer(&bufDesc, &subData, bufPtr.ReleaseAndGetAddressOf()))) {
			MLOG(LE, __FUNCTION__, LL, " create index buffer failed!");
			return BufferIdx(-1);
		}

		m_buffers.push_back(DX11_Buffer(numIdxs * IdxSize, numIdxs,
			BT_INDEX_BUFFER, flags, bufPtr, m_buffers.size()));
		return BufferIdx(m_buffers.size() - 1);
	}
}
