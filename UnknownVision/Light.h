#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include "UObject.h"

enum LightType {
	LT_DIRECTION = 0,
	LT_POINT
};

class LightProxy;

class Light {
	friend class LightProxy;
public:
	Light(LightType type);
	virtual void* GetLightData() = 0;
public:
	const LightType												m_type;
protected:
	LightProxy*														m_proxy;
	size_t																m_offset;
};

// 暂时一个灯光代理支持固定数量的灯光
// 方向光2

class LightProxy : public UObject {
public:
	static size_t MAX_DIRECTION_LIGHT;
	static size_t MAX_POINT_LIGHT;
	static size_t DATA_SIZE;
public:
	LightProxy(std::vector<Light*>&);
public:
	bool Setup(ID3D11Device*, ID3D11DeviceContext*);
	void Bind(ID3D11Device*, ID3D11DeviceContext*);
	void Unbind(ID3D11Device*, ID3D11DeviceContext*);

	void Update(Light*);
private:
	void updateBuffer(ID3D11DeviceContext*);
private:
	// x: Direction light, y: Point light
	DirectX::XMFLOAT4															m_numLight;
	std::vector<Light*>															m_dirLights;
	std::vector<Light*>															m_pointLights;
	std::vector<Light*>															m_updateList;

	Microsoft::WRL::ComPtr<ID3D11Buffer>							m_lightBuffer;
	std::shared_ptr<byte>														m_data;
};

class DirectionLight : public Light {
public:
	struct DL {
		DirectX::XMFLOAT4														m_color;
		DirectX::XMFLOAT4														m_direction;
		DirectX::XMFLOAT4														m_position;
	};
public:
	DirectionLight(DirectX::XMFLOAT3 direction, DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 position);
public:
	///////////////////
	// Light 
	///////////////////
	void* GetLightData();
public:
	void Rotate(DirectX::XMFLOAT3& rot);
	void Rotate(float x, float y, float z);

	void Translate(DirectX::XMFLOAT3& dir);
	void Translate(float x, float y, float z);

	void SetColorRGB(float r, float g, float b);
	void SetColorR(float r);
	void SetColorG(float g);
	void SetColorB(float b);

	DirectX::XMFLOAT4 GetColor() const;
private:
	void update();
private:
	DL																					m_data;
};

class PointLight : public Light {
public:
	struct PL {
		DirectX::XMFLOAT4														m_colorNradius;
		DirectX::XMFLOAT4														m_position;
	};
public:
	PointLight(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color, float radius);
public:
	///////////////////
	// Light
	///////////////////
	void* GetLightData();
public:
	void Translate(DirectX::XMFLOAT3& dir);
	void Translate(float x, float y, float z);
	void SetRadius(float r);
	void SetColorRGB(float r, float g, float b);
	void SetColorR(float r);
	void SetColorG(float g);
	void SetColorB(float b);

	DirectX::XMFLOAT3 GetColor() const;
private:
	void update();
private:
	PL																						m_data;
};
