#ifndef D3D11_TEXTURE_H
#define D3D11_TEXTURE_H

#include "../DX11_UVConfig.h"

namespace UnknownVision {
	class DX11_Texture2D : public UnknownVision::Texture2D {
	public:
		DX11_Texture2D(float width, float height,
			SmartPTR<ID3D11Texture2D> ptr,
			uint32_t flag, TextureElementType type, UINT RID)
			: m_tex(ptr), UnknownVision::Texture2D(width, height, flag, type, RID) {}

		ID3D11Texture2D* Texture() const { return m_tex.Get(); }
		ID3D11ShaderResourceView* ShaderResourceView() const { return m_srv.Get(); }
		ID3D11RenderTargetView* RenderTargetView() const { return m_rtv.Get(); }
		ID3D11DepthStencilView* DepthStencilView() const { return m_dsv.Get(); }
		ID3D11UnorderedAccessView* UnorderAccessView() const { return m_uav.Get(); }

		void SetShaderResourceView(SmartPTR<ID3D11ShaderResourceView>& ptr) { m_srv.Swap(ptr); }
		void SetRenderTargetView(SmartPTR<ID3D11RenderTargetView>& ptr) { m_rtv.Swap(ptr); }
		void SetDepthStencilView(SmartPTR<ID3D11DepthStencilView>& ptr) { m_dsv.Swap(ptr); }
		void SetUnorderAccessView(SmartPTR<ID3D11UnorderedAccessView>& ptr) { m_uav.Swap(ptr); }
	private:
		// 一个贴图可能有多个公用，所以需要多种view，运行时不保证view已经被正确初始化
		SmartPTR<ID3D11Texture2D> m_tex; // 实际存储贴图的对象
		SmartPTR<ID3D11ShaderResourceView> m_srv;
		SmartPTR<ID3D11RenderTargetView> m_rtv;
		SmartPTR<ID3D11DepthStencilView> m_dsv;
		SmartPTR<ID3D11UnorderedAccessView> m_uav;
	};
}

#endif // D3D11_TEXTURE_H
