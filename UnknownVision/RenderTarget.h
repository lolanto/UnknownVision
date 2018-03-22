#pragma once
#include "UnknownObject.h"

#include <d3d11.h>
#include <wrl.h>

class IRenderTarget {
public:
	virtual ID3D11RenderTargetView* GetRTV() = 0;
};

// ��ͨ��Ⱦ���󣬽������õ���Ⱦ�����װ��ͳһ�ӿ�

class CommonRenderTarget : public IRenderTarget {
public:
	CommonRenderTarget(ID3D11RenderTargetView*);
	CommonRenderTarget();
public:
	ID3D11RenderTargetView* GetRTV();

private:
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>					m_renderTarget;
};

class IUATarget {
public:
	virtual ID3D11UnorderedAccessView** GetUAV() = 0;
};
