
#include "Light.h"
#include "InfoLog.h"

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;

DirectionLight::DirectionLight(XMFLOAT3 direction, XMFLOAT3 color, XMFLOAT3 position)
	: Light(LT_DIRECTION) {
	memcpy_s(&m_data.m_color, sizeof(color), &color, sizeof(color));
	m_data.m_color.w = 0.0f;
	memcpy_s(&m_data.m_direction, sizeof(direction), &direction, sizeof(direction));
	m_data.m_direction.w = 0.0f;
	DirectX::XMStoreFloat4(&m_data.m_direction,
		DirectX::XMVector4Normalize(DirectX::XMLoadFloat4(&m_data.m_direction)));
	memcpy_s(&m_data.m_position, sizeof(position), &position, sizeof(position));
	m_data.m_position.w = 0.0f;
}

///////////////////
// public function
///////////////////

void* DirectionLight::GetLightData() {
	return &m_data;
}

void DirectionLight::Rotate(XMFLOAT3& rot) {
	Rotate(rot.x, rot.y, rot.z);
}

void DirectionLight::Rotate(float x, float y, float z) {
	static DirectX::XMVECTOR vector_dir;
	vector_dir = DirectX::XMLoadFloat4(&m_data.m_direction);
	// rotate the m_direcion
	vector_dir = DirectX::XMVector3Rotate(vector_dir, DirectX::XMQuaternionRotationRollPitchYaw(x, y,z));
	DirectX::XMStoreFloat4(&m_data.m_direction, vector_dir);
	update();
}

void DirectionLight::Translate(XMFLOAT3& dir) {
	static DirectX::XMVECTOR vector_pos;
	static DirectX::XMVECTOR vector_translate_dir;
	vector_pos = DirectX::XMLoadFloat4(&m_data.m_position);
	vector_translate_dir = DirectX::XMLoadFloat3(&dir);
	vector_pos = DirectX::XMVectorAdd(vector_pos, vector_translate_dir);
	DirectX::XMStoreFloat4(&m_data.m_position, vector_pos);
	update();
}

void DirectionLight::Translate(float x, float y, float z) {
	Translate(XMFLOAT3(x, y, z));
}

void DirectionLight::SetColorR(float r) {
	m_data.m_color.x = r;
	update();
}

void DirectionLight::SetColorG(float g) {
	m_data.m_color.y = g;
	update();
}

void DirectionLight::SetColorB(float b) {
	m_data.m_color.z = b;
	update();
}

void DirectionLight::SetColorRGB(float r, float g, float b) {
	m_data.m_color = DirectX::XMFLOAT4(r, g, b, 0.0f);
	update();
}

DirectX::XMFLOAT4 DirectionLight::GetColor() const {
	return m_data.m_color;
}

///////////////////
// private function
///////////////////

void DirectionLight::update() {
	m_proxy->Update(this);
}
