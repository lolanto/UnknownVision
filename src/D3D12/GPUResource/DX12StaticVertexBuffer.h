#pragma once
#include "../../GPUResource/Buffer.h"
#include "../DX12RenderDevice.h"
#include "../DX12ResourceManager.h"
#include "../DX12Config.h"

BEG_NAME_SPACE

template<typename T>
class DX12StaticVertexBuffer : public StaticVertexBuffer<T> {
public:
	DX12StaticVertexBuffer() {}
	VertexBufferView* GetVBVPtr() final {
		if (Avaliable()) return &m_vbv;
		return nullptr;
	}
	void* GetResource() override {
		return m_pVertexBuffer;
	}
	void SetName(const wchar_t* name) final {
		if (m_pVertexBuffer)
			m_pVertexBuffer->SetName(name);
	}
	bool Avaliable() const final {
		return m_pVertexBuffer != nullptr;
	}
	/** 请求临时资源，该资源会在对应的CommandUnit执行完指令后被释放 */
	virtual bool RequestTransient(CommandUnit* cmdUnit) final { return false; }
	/** 请求固定的资源，资源的释放需要手动控制 */
	virtual bool RequestPermenent(CommandUnit* cmdUnit) final;
	/** 用来手动释放资源，临时资源也可以提前进行手动释放，保证释放空资源不会有影响 */
	virtual void Release() final {
		StaticVertexBuffer::Release();
		assert(m_pResMgr->ReleaseResource(m_pVertexBuffer));
		m_pVertexBuffer = nullptr;
	}
private:
	ID3D12Resource* m_pVertexBuffer;
	DX12ResourceManager* m_pResMgr;
	DX12VertexBufferView m_vbv;
};

template<typename T>
bool DX12StaticVertexBuffer<T>::RequestPermenent(CommandUnit* cmdUnit) {
	/** TODO: 创建失败时需要清空之前的申请内容 */
	if (m_count == 0 || m_pData == nullptr) return false;
	const size_t sizeOfBufferInByte = m_count * sizeof(StaticVertexBuffer<T>::VertexDataType);
	DX12RenderDevice* dxDev = dynamic_cast<DX12RenderDevice*>(cmdUnit->GetDevice());
	if (dxDev == nullptr) return false;
	DX12ResourceManager& resMgr = dxDev->ResourceManager();
	auto [pRes, state] = resMgr.RequestBuffer(sizeOfBufferInByte,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_HEAP_TYPE_DEFAULT,
		true);
	if (pRes == nullptr) return false;
	m_pVertexBuffer = pRes;
	m_state = DX12ResourceStateToResourceState(state);
	m_pResMgr = &resMgr;
	/** 需要创建一个CPU可读的缓冲完成数据的拷贝 */
	{
		DX12CommandUnit* dxCmdUnit = dynamic_cast<DX12CommandUnit*>(cmdUnit);
		ID3D12Resource* pTmpRes = dxCmdUnit->TransientBuffer(sizeOfBufferInByte);
		void* mapData = nullptr;
		assert(SUCCEEDED(pTmpRes->Map(0, nullptr, &mapData)));
		memcpy(mapData, m_pData, sizeOfBufferInByte);
		pTmpRes->Unmap(0, nullptr);
		/** 向指令队列中添加拷贝指令 */
		dxCmdUnit->CopyBetweenBuffer(m_pVertexBuffer, pTmpRes);
	}
	m_vbv.m_view.BufferLocation = pRes->GetGPUVirtualAddress();
	m_vbv.m_view.SizeInBytes = sizeOfBufferInByte;
	m_vbv.m_view.StrideInBytes = sizeof(StaticVertexBuffer<T>::VertexDataType);
	m_bPermenent = true;
	return true;
}

END_NAME_SPACE
