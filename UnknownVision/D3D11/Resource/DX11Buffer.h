#ifndef D3D11_BUFFER_H
#define D3D11_BUFFER_H

#include "../DX11_UVConfig.h"

namespace UnknownVision {
	class DX11_VertexBuffer : public UnknownVision::VertexBuffer {
	public:
		DX11_VertexBuffer(size_t numVtxs, size_t sizeVtx,
			SmartPTR<ID3D11Buffer> buffer,
			BufferFlag flag, UINT RID) : m_buffer(buffer),
				VertexBuffer(numVtxs, sizeVtx, flag, RID) {}
		const SmartPTR<ID3D11Buffer>& Buffer() const { return m_buffer; }
	private:
		SmartPTR<ID3D11Buffer> m_buffer;
	};
}

#endif // D3D11_BUFFER_H
