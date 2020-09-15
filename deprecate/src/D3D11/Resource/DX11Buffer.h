#ifndef D3D11_BUFFER_H
#define D3D11_BUFFER_H

#include "../DX11_UVConfig.h"

namespace UnknownVision {
	class DX11_Buffer : public UnknownVision::Buffer {
	public:
		/// DX11_Buffer构造函数
		/** DX11_Buffer构造函数，主要设置DX11需要的缓冲区指针
		 * @param size 缓冲区占用字节总量
		 * @param numEle 缓冲区中元素的数量
		 * @param type 缓冲区的类型，详细定义查看BufferType
		 * @param flags 缓冲区的特性，详细定义查看BufferFlag
		 * @param bufferPtr 指向DX11缓冲区的指针，已经由管理器生成完毕
		 * @param RID 资源的索引号
		 */
		DX11_Buffer(size_t size, size_t numEle, BufferType type, 
			BufferFlagCombination flags,
			SmartPTR<ID3D11Buffer>& bufferPtr, uint32_t RID)
			: m_buffer(bufferPtr), Buffer(size, numEle, type, flags, RID) {}
		ID3D11Buffer* BufferPtr() const { return m_buffer.Get(); }
	private:
		SmartPTR<ID3D11Buffer> m_buffer;
	};
}

#endif // D3D11_BUFFER_H
