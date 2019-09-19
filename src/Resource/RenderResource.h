#pragma once

#include "../UVType.h"
#include "ShaderResource.h"

BEG_NAME_SPACE

class CommandUnit;
class RenderDevice;

class RenderResource {
public:
	virtual ConstantBufferView* GetCBVPtr() { return nullptr; }
	virtual ShaderResourceView* GetSRVPtr() { return nullptr; }
	virtual UnorderAccessView* GetUAVPtr() { return nullptr; }
	virtual RenderTargetView* GetRTVPtr() { return nullptr; }
	virtual DepthStencilView* GetDSVPtr() { return nullptr; }
	virtual bool Avaliable() const { return false; }
	/** 请求临时资源，该资源会在对应的CommandUnit执行完指令后被释放 */
	virtual bool RequestTransient(CommandUnit* cmdUnit, ResourceStatus status) { return false; };
	/** 请求固定的资源，资源的释放需要手动控制 */
	virtual bool RequestPermenent(RenderDevice* cmdUnit, ResourceStatus status) { return false; };
	/** 用来手动释放资源，临时资源也可以提前进行手动释放，保证释放空资源不会有影响 */
	virtual void Release() = 0;
protected:
	ResourceStatus m_status;
};

class Texture : public RenderResource {
public:
	bool IsTexture1D() const { return m_width != 0 && m_height == 0 && m_slice == 0; }
	bool IsTexture2D() const { return m_width != 0 && m_height != 0 && m_slice == 0; }
	bool IsTexture3D() const { return m_width != 0 && m_height != 0 && m_slice != 0; }
public:
	size_t& Width() { return m_width; }
	size_t Width() const { return m_width; }
	size_t& Height() { return m_height; }
	size_t Height() const { return m_height; }
	size_t& Slice() { return m_slice; }
	size_t Slice() const { return m_slice; }
protected:
	size_t m_width;
	size_t m_height;
	size_t m_slice;
};

class Buffer : public RenderResource {
public:
	virtual void Reset() = 0;
	size_t& Size() { return m_size; }
	size_t Size() const { return m_size; }
protected:
	size_t m_size;
};

END_NAME_SPACE
