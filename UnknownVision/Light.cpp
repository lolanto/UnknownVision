#include "Light.h"
#include "InfoLog.h"

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Light   /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

Light::Light(LightType type)
	: m_type(type){}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   LightProxy   ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

size_t LightProxy::MAX_DIRECTION_LIGHT = 2;
size_t LightProxy::MAX_POINT_LIGHT = 2;
size_t LightProxy::DATA_SIZE = sizeof(XMFLOAT4) + 
	sizeof(DirectionLight::DL) * LightProxy::MAX_DIRECTION_LIGHT +
	sizeof(PointLight::PL) * LightProxy::MAX_POINT_LIGHT;

LightProxy::LightProxy(std::vector<Light*>& light) {
	m_data.reset(new byte[DATA_SIZE]);
	m_numLight = XMFLOAT4(0, 0, 0, 0);
	for (auto iter = light.begin(), end = light.end(); iter != end; ++iter) {
		switch ((*iter)->m_type) {
		case LT_DIRECTION:
			if (m_numLight.x <= MAX_DIRECTION_LIGHT) {
				m_dirLights.push_back(*iter);
				m_updateList.push_back(*iter);
				(*iter)->m_proxy = this;
				(*iter)->m_offset = sizeof(m_numLight) + sizeof(DirectionLight::DL) * m_numLight.x;
				++m_numLight.x;
			}
			break;
		case LT_POINT:
			if (m_numLight.y <= MAX_POINT_LIGHT) {
				m_pointLights.push_back(*iter);
				m_updateList.push_back(*iter);
				(*iter)->m_proxy = this;
				(*iter)->m_offset = sizeof(m_numLight) + sizeof(DirectionLight::DL) * MAX_DIRECTION_LIGHT;
				(*iter)->m_offset += sizeof(PointLight::PL) * m_numLight.y;
				++m_numLight.y;
			}
			break;
		}
	}
}

///////////////////
// public function
///////////////////

bool LightProxy::Setup(ID3D11Device* dev, ID3D11DeviceContext* devCtx) {
	static const char* funcTag = "LightProxy::Setup: ";
	// create buffer but no set data
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA subData;
	ZeroMemory(&desc, sizeof(desc));
	ZeroMemory(&subData, sizeof(subData));
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.ByteWidth = DATA_SIZE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DYNAMIC;

	if (FAILED(dev->CreateBuffer(&desc, NULL, m_lightBuffer.ReleaseAndGetAddressOf()))) {
		MLOG(LL, funcTag, LL, "create light data buffer failed!");
		return false;
	}

	// initialize data
	updateBuffer(devCtx);
	return true;
}

void LightProxy::Bind(ID3D11Device* dev, ID3D11DeviceContext* devCtx) {
	if (!m_lightBuffer.Get()) {
		MLOG(LL, "LightProxy::Bind: has not setup!");
		return;
	}
	if (m_updateList.size()) {
		updateBuffer(devCtx);
	}
	devCtx->PSSetConstantBuffers(1, 1, m_lightBuffer.GetAddressOf());
}

void LightProxy::Unbind(ID3D11Device* dev, ID3D11DeviceContext* devCtx) {
	static ID3D11Buffer* nullPtr[] = { NULL };
	devCtx->PSSetConstantBuffers(1, 1, nullPtr);
}

void LightProxy::Update(Light* l) {
	m_updateList.push_back(l);
}

///////////////////
// private function
///////////////////

void LightProxy::updateBuffer(ID3D11DeviceContext* devCtx) {
	static size_t size = -1;
	byte* vData = m_data.get();
	memcpy_s(vData, sizeof(m_numLight), &m_numLight, sizeof(m_numLight));
	for (auto iter = m_updateList.begin(), end = m_updateList.end(); iter != end; ++iter) {
		switch ((*iter)->m_type) {
		case LT_DIRECTION:
			size = sizeof(DirectionLight::DL);
			break;
		case LT_POINT:
			size = sizeof(PointLight::PL);
			break;
		default:
			continue;
		}
		memcpy_s(vData + (*iter)->m_offset, size, (*iter)->GetLightData(), size);
	}
	D3D11_MAPPED_SUBRESOURCE map;
	if (FAILED(devCtx->Map(m_lightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) {
		MLOG(LL, "LightProxy::updateBuffer: update buffer failed!");
		return;
	}
	memcpy_s(map.pData, DATA_SIZE, m_data.get(), DATA_SIZE);
	devCtx->Unmap(m_lightBuffer.Get(), 0);
	m_updateList.clear();
}
