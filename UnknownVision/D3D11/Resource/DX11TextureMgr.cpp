#include "DX11ResMgr.h"

namespace UnknownVision {
	int DX11_Texture2DMgr::Create(float width, float height,
		uint32_t flag, TextureElementType type, uint8_t* data, size_t size) {
		return -1;
	}

	int DX11_Texture2DMgr::CreateTexture(float width, float height,
		TextureElementType type, uint8_t* data, size_t bytePerLine) {
		// 创建贴图必须附上纹理数据
		if (data == nullptr) return -1;
		D3D11_TEXTURE2D_DESC&& texDesc = createTexDesc(width, height, type);
		D3D11_SUBRESOURCE_DATA initData;
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		memset(&initData, 0, sizeof(D3D11_SUBRESOURCE_DATA));
		memset(&srvDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		// 初始化数据规格
		initData.pSysMem = data;
		initData.SysMemPitch = bytePerLine;

		SmartPTR<ID3D11Texture2D> texPtr;
		if (FAILED(m_dev->CreateTexture2D(&texDesc, &initData, texPtr.GetAddressOf()))) {
			assert(false);
			return -1;
		}

		// 构造shader resource
		srvDesc.Texture2D.MipLevels = -1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = texDesc.Format;
		SmartPTR<ID3D11ShaderResourceView> srvPtr;
		if (FAILED(m_dev->CreateShaderResourceView(texPtr.Get(), &srvDesc, srvPtr.GetAddressOf()))) {
			assert(false);
			return -1;
		}

		m_texs.push_back(DX11_Texture2D(width, height, texPtr,
			TF_ONLY_READ, type, 0));
		m_texs.back().SetShaderResourceView(srvPtr);
		return m_texs.size() - 1;
	}

	int DX11_Texture2DMgr::CreateRenderTarget(float width, float height,
		TextureElementType type) {
		D3D11_TEXTURE2D_DESC&& texDesc = createTexDesc(width, height, type);
		D3D11_RENDER_TARGET_VIEW_DESC srvDesc;
		memset(&srvDesc, 0, sizeof(srvDesc));
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

		SmartPTR<ID3D11Texture2D> texPtr;
		if (FAILED(m_dev->CreateTexture2D(&texDesc, nullptr, texPtr.GetAddressOf()))) {
			assert(false);
			return -1;
		}

		// 设置render target view属性
		srvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		srvDesc.Format = texDesc.Format;
		srvDesc.Texture2D.MipSlice = 0;
		SmartPTR<ID3D11RenderTargetView> rtvPtr;
		if (FAILED(m_dev->CreateRenderTargetView(texPtr.Get(), &srvDesc, rtvPtr.GetAddressOf()))) {
			assert(false);
			return -1;
		}

		m_texs.push_back(DX11_Texture2D(width, height, texPtr,
			TF_WRITE, type, 0));
		m_texs.back().SetRenderTargetView(rtvPtr);
		return m_texs.size() - 1;
	}

	int DX11_Texture2DMgr::CreateDepthStencilTexture(float width, float height,
		TextureElementType type) {
		D3D11_TEXTURE2D_DESC&& texDesc = createTexDesc(width, height, type);
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		memset(&dsvDesc, 0, sizeof(dsvDesc));
		texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		SmartPTR<ID3D11Texture2D> texPtr;
		if (FAILED(m_dev->CreateTexture2D(&texDesc, nullptr, texPtr.GetAddressOf()))) {
			assert(false);
			return -1;
		}
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = texDesc.Format;
		dsvDesc.Texture2D.MipSlice = 0;
		// 该属性用于设置当前view是否只读，若只读则可以同时绑定多个view
		// 暂时不对该属性进行限制，管线依旧可以对view进行写入
		dsvDesc.Flags = 0;
		SmartPTR<ID3D11DepthStencilView> dsvPtr;
		if (FAILED(m_dev->CreateDepthStencilView(texPtr.Get(), &dsvDesc, dsvPtr.GetAddressOf()))) {
			assert(false);
			return -1;
		}
		m_texs.push_back(DX11_Texture2D(width, height, texPtr,
			TF_DEPTH_STENCIL | TF_WRITE, type, 0));
		m_texs.back().SetDepthStencilView(dsvPtr);
		return m_texs.size() - 1;
	}

	D3D11_TEXTURE2D_DESC DX11_Texture2DMgr::createTexDesc(float width, float height,
		TextureElementType type) {
		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.ArraySize = 1; // 暂时没有考虑texture array
		desc.CPUAccessFlags = 0; // CPU不需要访问纹理内容
		DXGI_FORMAT format;
		switch (type) {
		case UNORM_R8G8B8: // 当读取24位像素时，每个像素需要扩展8位
		case UNORM_R8G8B8A8:
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case UNORM_D24_UINT_S8:
			format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		default:
			assert(false);
		}
		desc.Format = format;
		desc.Height = static_cast<UINT>(height);
		desc.Width = static_cast<UINT>(width);
		desc.MipLevels = 1; // 暂时不考虑mipmap
							// texDesc.MiscFlags = 0; // 使用mipmap的时候需要设置flag，从而生成mipmap
							// 禁止对贴图多重采样
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		return desc;
	}
}