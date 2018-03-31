#pragma once
#include <d3d11.h>
#include <wrl.h>
#include "UnknownObject.h"

/* 资源抽象接口
1. 创建
2. 在管线中绑定
3. 从管线中移除
4. 资源类型
	目前已知类型：
	1. 贴图：
		(RW)Texture2D(Array) UA
	2. 立方体贴图
		TextureCube(Array size = 6) UA
	3. 常量缓冲区
		constant buffer
	4. 结构化缓冲区
		(RW)StructuredBuffer UA
	5. 采样器
		SamplerState
	6. 渲染对象
		RenderTarget

一个实体资源(resource)同时能成为多种类型资源(view)
但同时只能绑定一种类型

抽象概念
创建——不同类型还需要考虑不同的创建参数
绑定/解除绑定——不同类型涉及到不同的绑定函数调用
*/

class IConstantBuffer {
public:
	virtual void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	virtual void Unbind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
protected:
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_buf;
};

class ITexture {
public:
	virtual void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	virtual void Unbind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	virtual void GenMipMap(ID3D11DeviceContext*);
protected:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_srv_tex;
};

class IBuffer {
public:
	virtual void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	virtual void Unbind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
protected:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_srv_buf;
};

class IUnorderAccess {
public:
	virtual void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	virtual void Unbind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
protected:
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>		m_uav;
};

class ISamplerState {
public:
	virtual void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	virtual void Unbind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
protected:
	Microsoft::WRL::ComPtr<ID3D11SamplerState>					m_sampler;
};

class IRenderTarget {
public:
	virtual ID3D11RenderTargetView* GetRTV();
protected:
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>			m_rtv;
};

class IDepthStencil {
public:
	virtual ID3D11DepthStencilView* GetDSV();
protected:
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>				m_dsv;
};

class RenderTargetWrapper : public IRenderTarget {
public:
	RenderTargetWrapper(ID3D11RenderTargetView*);
};
