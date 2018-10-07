#pragma once
#include "../ResourceManager.h"
#include "DX11_UVConfig.h"

class DX11_CanvasManager : public CanvasManager {
public:
	DX11_CanvasManager(SmartPTR<ID3D11Device>& dev,
		SmartPTR<ID3D11DeviceContext>& devCtx)
		: m_dev(dev), m_devCtx(devCtx) {}
public:
	void GetResourceData(_In_  Resource* re, _Inout_ NameParams& param);
	// ��ָ��·������ͼƬ���������ô�С
	Canvas Create(std::string name, UINT usage, char* path = nullptr);
	// �����ض��ߴ��ͼƬ������
	Canvas Create(std::string name, UINT usage, float width, float height);
private:
	SmartPTR<ID3D11Device>									m_dev;
	SmartPTR<ID3D11DeviceContext>						m_devCtx;
	DX11_UV::CanvasDataContainer							m_datas;
};