#pragma once

#include <memory>
#include <map>
#include <wrl.h>
#include "InfoLog.h"
#include "UnknownObject.h"
#include "Resource.h"
template<typename T>
class ConstantBuffer : public IConstantBuffer {
public:
	ConstantBuffer() : m_isDirty(true) {}
	ConstantBuffer(T& t) : m_data(t), m_isDirty(true) {}
	bool Setup(ID3D11Device*);
public:
	void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	// Get and update data
	T & GetData() { m_isDirty = true; return m_data; }
	// Read data but no update
	const T & ReadData() const { return m_data; }
private:
	T																		m_data;
	bool																	m_isDirty;
};

template<typename T>
bool ConstantBuffer<T>::Setup(ID3D11Device* dev) {
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA subData;
	ZeroMemory(&desc, sizeof(desc));
	ZeroMemory(&subData, sizeof(subData));

	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.ByteWidth = sizeof(T);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DYNAMIC;

	subData.pSysMem = &m_data;
	subData.SysMemPitch = 0;
	subData.SysMemSlicePitch = 0;
	if (FAILED(dev->CreateBuffer(&desc, &subData, m_buf.ReleaseAndGetAddressOf()))) {
		MLOG(LE, __FUNCTION__, LL, "create constant buffer failed!");
		return false;
	}
	return true;
}

template<typename T>
void  ConstantBuffer<T>::Bind(ID3D11DeviceContext* devCtx, 
	ShaderBindTarget sbt, SIZE_T slot) {
	if (m_isDirty) {
		// update data
		D3D11_MAPPED_SUBRESOURCE map;
		devCtx->Map(m_buf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
		memcpy_s(map.pData, sizeof(T), &m_data, sizeof(T));
		devCtx->Unmap(m_buf.Get(), 0);
		m_isDirty = false;
	}
	IConstantBuffer::Bind(devCtx, sbt, slot);
}


template<typename T, int NumEle>
class StructuredBuffer : public IBuffer, public IUnorderAccess {
public:
	StructuredBuffer(bool isReadOnly = true)
		: isUnoderAccess(!isReadOnly) {}
	bool Setup(ID3D11Device*);
public:
	const bool														isUnoderAccess;
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_buf;
};

template<typename T, int NumEle>
bool StructuredBuffer<T, NumEle>::Setup(ID3D11Device* dev) {
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	if (isUnoderAccess) desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	desc.ByteWidth = sizeof(T) * NumEle;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(T);
	desc.Usage = D3D11_USAGE_DEFAULT;

	if (FAILED(dev->CreateBuffer(&desc, nullptr, m_buf.ReleaseAndGetAddressOf()))) {
		MLOG(LE, __FUNCTION__, LL, "create structured buffer failed!");
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.ElementOffset = 0;
	srvDesc.Buffer.NumElements = NumEle;
	if (FAILED(dev->CreateShaderResourceView(m_buf.Get(), &srvDesc, m_srv_buf.ReleaseAndGetAddressOf()))) {
		MLOG(LE, __FUNCTION__, LL, "create shader resource view failed!");
		return false;
	}

	if (isUnoderAccess) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory(&uavDesc, sizeof(uavDesc));
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = NumEle;
		// 在uav中添加隐藏计数器并激活其功能
		uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		if (FAILED(dev->CreateUnorderedAccessView(m_buf.Get(), &uavDesc, m_uav.ReleaseAndGetAddressOf()))) {
			MLOG(LE, __FUNCTION__, LL, "create unorder access view failed!");
			return false;
		}
	}

	return true;
}