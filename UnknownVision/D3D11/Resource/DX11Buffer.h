#ifndef D3D11_BUFFER_H
#define D3D11_BUFFER_H

#include "../DX11_UVConfig.h"

namespace UnknownVision {
	class DX11_Buffer : public UnknownVision::Buffer {
	public:
		DX11_Buffer(size_t size, size_t numEle, uint32_t flag,
			SmartPTR<ID3D11Buffer>& bufferPtr, uint32_t RID)
			: m_buffer(bufferPtr), Buffer(size, numEle, flag, RID) {}
		ID3D11Buffer* BufferPtr() const { return m_buffer.Get(); }
	private:
		SmartPTR<ID3D11Buffer> m_buffer;
	};
}

#endif // D3D11_BUFFER_H
