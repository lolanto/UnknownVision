#include "Light.h"

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;

PointLight::PointLight(XMFLOAT3 position, XMFLOAT3 color, float radius) 
	: Light(LT_POINT){
	memcpy_s(&m_data.m_position, sizeof(XMFLOAT3), &position, sizeof(XMFLOAT3));
	m_data.m_position.w = 0.0f;
	memcpy_s(&m_data.m_colorNradius, sizeof(XMFLOAT3), &color, sizeof(XMFLOAT3));
	m_data.m_colorNradius.w = radius;
}

///////////////////
// public function
///////////////////

void* PointLight::GetLightData() { return &m_data; }

void PointLight::Translate(XMFLOAT3& dir) {
	static DirectX::XMVECTOR vector_dir;
	vector_dir = DirectX::XMLoadFloat3(&dir);
	DirectX::XMStoreFloat4(&m_data.m_position, DirectX::XMVectorAdd(
		vector_dir, DirectX::XMLoadFloat4(&m_data.m_position)
	));
	update();
}

void PointLight::Translate(float x, float y, float z) {
	Translate(XMFLOAT3(x, y, z));
}

void PointLight::SetRadius(float r) {
	m_data.m_colorNradius.w = r;
	update();
}

void PointLight::SetColorRGB(float r, float g, float b) {
	m_data.m_colorNradius.x = r;
	m_data.m_colorNradius.y = g;
	m_data.m_colorNradius.z = b;
	update();
}

void PointLight::SetColorR(float r) {
	m_data.m_colorNradius.x = r;
	update();
}

void PointLight::SetColorG(float g) {
	m_data.m_colorNradius.y = g;
	update();
}

void PointLight::SetColorB(float b) {
	m_data.m_colorNradius.z = b;
	update();
}

XMFLOAT3 PointLight::GetColor() const { return XMFLOAT3(
	m_data.m_colorNradius.x,
	m_data.m_colorNradius.y,
	m_data.m_colorNradius.z); }

///////////////////
// private function
///////////////////

void PointLight::update() {
	m_proxy->Update(this);
}
