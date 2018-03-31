#pragma once
#include <d3d11.h>
#include <wrl.h>
#include "UnknownObject.h"

/* ��Դ����ӿ�
1. ����
2. �ڹ����а�
3. �ӹ������Ƴ�
4. ��Դ����
	Ŀǰ��֪���ͣ�
	1. ��ͼ��
		(RW)Texture2D(Array) UA
	2. ��������ͼ
		TextureCube(Array size = 6) UA
	3. ����������
		constant buffer
	4. �ṹ��������
		(RW)StructuredBuffer UA
	5. ������
		SamplerState
	6. ��Ⱦ����
		RenderTarget

һ��ʵ����Դ(resource)ͬʱ�ܳ�Ϊ����������Դ(view)
��ͬʱֻ�ܰ�һ������

�������
����������ͬ���ͻ���Ҫ���ǲ�ͬ�Ĵ�������
��/����󶨡�����ͬ�����漰����ͬ�İ󶨺�������
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
