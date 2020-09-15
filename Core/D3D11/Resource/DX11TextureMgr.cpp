#include "../../UVRoot.h"
#include "../DX11RenderSys.h"
#include "DX11ResMgr.h"

namespace UnknownVision {

	Texture2DIdx DX11_Texture2DMgr::CreateTexture(float width, float height,
		TextureFlagCombination flags, TextureElementType type, uint8_t* data, size_t bytePerLine) {
		// CPU，GPU都不可以再重写纹理内容时，必须提供初始数据(比如创建普通贴图)
		if (!(flags & (TextureFlag::TF_WRITE_BY_CPU | TextureFlag::TF_WRITE_BY_GPU))
			&& data == nullptr) {
			MLOG(LE, __FUNCTION__, LL, " texture must has initial data since nobody can write it again");
			return Texture2DIdx (-1);
		}
		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.ArraySize = 1; // 暂时没有考虑texture array
		desc.CPUAccessFlags = 0;
		if (flags & TextureFlag::TF_READ_BY_CPU) desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
		if (flags & TextureFlag::TF_WRITE_BY_CPU) desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
		desc.Format = TextureElementType2DXGI_FORMAT(type);
		desc.Height = static_cast<UINT>(height);
		desc.Width = static_cast<UINT>(width);
		desc.MipLevels = 1; // 暂时不考虑使用mipmap
		desc.MiscFlags = 0; // 该值通常在mipmap使用时才需要特殊设置
		// 禁止对贴图多重采样
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		// GPU只读的资源，比如普通贴图
		if (!(flags & ~TextureFlag::TF_READ_BY_GPU) && (flags & TextureFlag::TF_READ_BY_GPU))
			desc.Usage = D3D11_USAGE_IMMUTABLE;
		// GPU负责读，CPU负责写。适合于需要cpu频繁修改，交由GPU处理的资源
		else if (flags & TextureFlag::TF_WRITE_BY_CPU && flags & TextureFlag::TF_READ_BY_GPU)
			desc.Usage = D3D11_USAGE_DYNAMIC;
		// 暂时不考虑D3D11_USAGE_STAGING
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		if (flags & TextureFlag::TF_WRITE_BY_GPU) {
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			// 只有类型符合要求的缓冲才能作为深度模板缓存
			if (static_cast<uint32_t>(type) >= static_cast<uint32_t>(TextureElementType::UNORM_D24_UINT_S8))
				desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		}

		D3D11_SUBRESOURCE_DATA initData;
		ID3D11Device* dev = static_cast<DX11_RenderSys&>(Root::GetInstance().GetRenderSys()).GetDevice();

		memset(&initData, 0, sizeof(D3D11_SUBRESOURCE_DATA));

		// 初始化数据规格
		initData.pSysMem = data;
		initData.SysMemPitch = bytePerLine;

		SmartPTR<ID3D11Texture2D> texPtr;

		if (FAILED(dev->CreateTexture2D(&desc, &initData, texPtr.GetAddressOf()))) {
			MLOG(LL, __FUNCTION__, LE, " Create Texture 2D Failed");
			return Texture2DIdx (-1);
		}

		m_texs.push_back({ width, height, texPtr, flags, type, m_texs.size() });
		return Texture2DIdx(m_texs.size() - 1);
	}

	RenderTargetIdx DX11_Texture2DMgr::CreateRenderTargetFromTexture(Texture2DIdx index) {
		assert(index >= 0 && static_cast<uint32_t>(index.value()) < m_texs.size());
		ID3D11Device* dev = static_cast<DX11_RenderSys&>(Root::GetInstance().GetRenderSys()).GetDevice();
		assert(dev != nullptr);
		const DX11_Texture2D& texture = m_texs[index.value()];
		assert(texture.Flag() & TextureFlag::TF_WRITE_BY_GPU);
		D3D11_RENDER_TARGET_VIEW_DESC srvDesc;
		memset(&srvDesc, 0, sizeof(srvDesc));

		// 设置render target view属性
		srvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		srvDesc.Format = TextureElementType2DXGI_FORMAT(texture.ElementType());
		// TODO: 一旦使用mipmap这里的设置可能需要更改
		srvDesc.Texture2D.MipSlice = 0; // 暂时还不使用mipmap，故设置默认值0
		SmartPTR<ID3D11RenderTargetView> rtvPtr;
		if (FAILED(dev->CreateRenderTargetView(texture.Texture(), &srvDesc, rtvPtr.GetAddressOf()))) {
			MLOG(LL, __FUNCTION__, LE, " create Render Target Failed!");
			return RenderTargetIdx(-1);
		}

		/* 将创建的RTV放入队列当中
			并建立RTV与Texture之间索引的映射关系
			最后返回RTV的索引
		**/
		m_rtvs.push_back(rtvPtr);
		m_texId2RTId.insert(std::make_pair(index, m_rtvs.size() - 1));
		return RenderTargetIdx(m_rtvs.size() - 1);
	}

	DepthStencilIdx DX11_Texture2DMgr::CreateDepthStencilFromTexture(Texture2DIdx index) {
		assert(index >= 0 && static_cast<uint32_t>(index.value()) < m_texs.size());
		ID3D11Device* dev = static_cast<DX11_RenderSys&>(Root::GetInstance().GetRenderSys()).GetDevice();
		assert(dev != nullptr);
		const DX11_Texture2D& texture = m_texs[index.value()];
		assert((texture.Flag() & TextureFlag::TF_WRITE_BY_GPU)
			&& static_cast<uint32_t>(texture.ElementType()) >= static_cast<uint32_t>(TextureElementType::UNORM_D24_UINT_S8));
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		memset(&dsvDesc, 0, sizeof(dsvDesc));

		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = TextureElementType2DXGI_FORMAT(texture.ElementType());
		// TODO: 一旦使用mipmap，该值可能需要重新设置
		dsvDesc.Texture2D.MipSlice = 0; // 暂时不适用Mipmap，故保持默认设置
		// 该属性用于设置当前view是否只读，若只读则可以同时绑定多个view
		// 暂时不对该属性进行限制，管线依旧可以对view进行写入
		dsvDesc.Flags = 0;
		SmartPTR<ID3D11DepthStencilView> dsvPtr;
		if (FAILED(dev->CreateDepthStencilView(texture.Texture(), &dsvDesc, dsvPtr.GetAddressOf()))) {
			MLOG(LL, __FUNCTION__, LE, "create depth stencil View failed!");
			return DepthStencilIdx(-1);
		}
		/* 创建DSV并添加到队列当中
			创建Texture到DSV的映射关系
			最后返回DSV索引
		**/
		m_dsvs.push_back(dsvPtr);
		m_texId2RTId.insert(std::make_pair(index, m_dsvs.size() - 1));
		return DepthStencilIdx(m_dsvs.size() - 1);
	}

	ID3D11RenderTargetView* DX11_Texture2DMgr::GetRenderTargetFromTexture(Texture2DIdx idx) {
		if (idx >= 0 && idx < m_texs.size()) {
			const auto& iter = m_texId2RTId.find(idx);
			if (iter != m_texId2RTId.end())
				return m_rtvs[iter->second.value()].Get();
			else {
				MLOG(LW, __FUNCTION__, LL, " RenderTarget is not found!");
				return nullptr;
			}
		}
		else {
			MLOG(LW, __FUNCTION__, LL, " idx is out of range!");
			return nullptr;
		}
	}

	ID3D11RenderTargetView* DX11_Texture2DMgr::GetRenderTargetFromTexture(RenderTargetIdx idx) {
		if (idx >= 0 && idx < m_rtvs.size()) {
			return m_rtvs[idx.value()].Get();
		}
		else {
			MLOG(LW, __FUNCTION__, LL, " idx is out of range!");
			return nullptr;
		}
	}

	ID3D11DepthStencilView* DX11_Texture2DMgr::GetDepthStencilFromTexture(Texture2DIdx idx) {
		if (idx >= 0 && idx < m_texs.size()) {
			const auto& iter = m_texId2DSId.find(idx);
			if (iter != m_texId2DSId.end()) {
				return m_dsvs[iter->second.value()].Get();
			}
			else {
				MLOG(LW, __FUNCTION__, LL, " depth stencil is not found!");
				return nullptr;
			}
		}
		else {
			MLOG(LW, __FUNCTION__, LL, " idx is out of range!");
			return nullptr;
		}
	}

	ID3D11DepthStencilView* DX11_Texture2DMgr::GetDepthStencilFromTexture(DepthStencilIdx idx) {
		if (idx >= 0 && idx < m_dsvs.size()) {
			return m_dsvs[idx.value()].Get();
		}
		else {
			MLOG(LW, __FUNCTION__, LL, " idx is out of range!");
			return nullptr;
		}
	}
}
