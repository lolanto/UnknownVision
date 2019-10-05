#pragma once
#include "../../GPUResource/Canvas2D.h"
#include "../DX12RenderDevice.h"
#include "../DX12ResourceManager.h"
#include "../DX12Config.h"

BEG_NAME_SPACE

class DX12DiscreteCanvas2D : public DiscreteCanvas2D {
public:
	void* GetResource() override {
		if (m_pCanvas) return m_pCanvas;
	}
	void SetName(const wchar_t* name) final {
		if (m_pCanvas) m_pCanvas->SetName(name);
	}
	RenderTargetView* GetRTVPtr() override { return &m_rtv; }
	ShaderResourceView* GetSRVPtr() override { return &m_srv; }
	virtual bool Avaliable() const { return m_pCanvas != nullptr; }
	/** 请求临时资源，该资源会在对应的CommandUnit执行完指令后被释放 */
	virtual bool RequestTransient(CommandUnit* cmdUnit) { return false; };
	/** 请求固定的资源，资源的释放需要手动控制 */
	virtual bool RequestPermenent(CommandUnit* cmdUnit) {
		/** TODO: 创建失败时需要清空之前的申请内容 */
		if (m_width == 0 || m_height == 0 || m_format == ELEMENT_FORMAT_TYPE_INVALID) return false;

		DX12RenderDevice* dxDev = dynamic_cast<DX12RenderDevice*>(cmdUnit->GetDevice());
		if (dxDev == nullptr) return false;
		DX12ResourceManager& resMgr = dxDev->ResourceManager();
		auto[pRes, state] = resMgr.RequestTexture(m_width, m_height, 0,
			ElementFormatToDXGIFormat(m_format),
			D3D12_RESOURCE_FLAG_NONE,
			0,
			true);
		if (pRes == nullptr) return false;
		m_pCanvas = pRes;
		m_state = DX12ResourceStateToResourceState(state);
		m_pResMgr = &resMgr;
		
		m_rtv.m_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		m_rtv.m_desc.Format = ElementFormatToDXGIFormat(m_format);
		m_rtv.m_desc.Texture2D.MipSlice = 0;
		m_rtv.m_desc.Texture2D.PlaneSlice = 0;
		m_rtv.m_res = m_pCanvas;

		m_srv.m_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		m_srv.m_desc.Format = ElementFormatToDXGIFormat(m_format);
		m_srv.m_desc.Texture2D.MipLevels = 1;
		m_srv.m_desc.Texture2D.MostDetailedMip = 0;
		m_srv.m_desc.Texture2D.PlaneSlice = 0;
		m_srv.m_desc.Texture2D.ResourceMinLODClamp = 0;
		m_srv.m_res = m_pCanvas;
		return true;
	}
	/** 用来手动释放资源，临时资源也可以提前进行手动释放，保证释放空资源不会有影响 */
	virtual void Release() {
		if (m_pCanvas == nullptr) return;
		m_pResMgr->ReleaseResource(m_pCanvas);
		m_pCanvas = nullptr;
		DiscreteCanvas2D::Release();
	}
private:
	ID3D12Resource* m_pCanvas;
	DX12ResourceManager* m_pResMgr;
	DX12RenderTargetView m_rtv;
	DX12ShaderResourceView m_srv;
};

END_NAME_SPACE
