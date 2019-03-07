#ifndef UV_D3D12_BUFFER_H
#define UV_D3D12_BUFFER_H

#include "../DX12Config.h"
#include "../../Resource/Buffer.h"

namespace UnknownVision {
	class DX12_Buffer : public Buffer {
	public:
		/** DX12缓冲区对象的构造函数
		 * @param buffer 需要该对象托管的缓冲区Com指针 
		 * @param byteSize 缓冲区的总大小 
		 * @param numEle 缓冲区中元素的数量 
		 * @param type 缓冲区类型(vb, idxb, constb..) 
		 * @param flag 缓冲区的读写权限设置
		 * @param RID 资源识别码 */
		DX12_Buffer(SmartPTR<ID3D12Resource1>& buffer,
			size_t byteSize, size_t numEle, BufferType type, BufferFlagCombination flag, uint32_t RID)
			: Buffer(byteSize, numEle, type, flag, RID), m_buffer(buffer) {}
		ID3D12Resource1* BufferPtr() const { return m_buffer.Get(); }
	private:
		SmartPTR<ID3D12Resource1> m_buffer; /**< 缓冲区资源指针 */
	};
}

#endif // UV_D3D12_BUFFER_H
