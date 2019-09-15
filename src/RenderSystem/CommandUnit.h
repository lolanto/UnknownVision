#pragma once
#include "../UVType.h"
#include <vector>
#include <array>
BEG_NAME_SPACE

class Buffer;

/** 资源应用类指令的接口集合 */
class CommandUnit {
public:

	virtual ~CommandUnit() = default;
public:
	virtual bool Active() = 0;
	virtual bool Fetch() = 0;
	virtual bool FetchAndPresent() = 0;
	virtual bool Wait() = 0;
	virtual bool Reset() = 0;
	/** 仅针对CPU可写的缓冲 */
	virtual bool UpdateBufferWithSysMem(BufferHandle dest, void* src, size_t size) = 0;
	/** 仅针对可读回的，以及CPU可读的缓冲 */
	virtual bool ReadBackToSysMem(BufferHandle src, void* dest, size_t size) = 0;
	virtual bool CopyBetweenGPUBuffer(BufferHandle src, BufferHandle dest, size_t srcOffset, size_t destOffset, size_t size) = 0;
	virtual bool TransferState(BufferHandle buf, ResourceStates newState) = 0;
	virtual bool TransferState(TextureHandle tex, ResourceStates newState) = 0;
	virtual bool TransferState(Buffer* buf, ResourceStates newState) = 0;
	//virtual bool UseProgram(ProgramHandle program) = 0;

	//virtual bool BindVertexBuffers(const std::vector<BufferHandle>& vtxBuffers) = 0;
	//virtual bool BindIndexBuffer(BufferHandle buf) = 0;
	virtual bool BindRenderTargetsAndDepthStencilBuffer(const std::vector<TextureHandle>& renderTargets, TextureHandle depthStencil) = 0;
	virtual bool ClearRenderTarget(TextureHandle renderTarget, const std::array<float, 4>& color) = 0;
};

END_NAME_SPACE
