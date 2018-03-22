#pragma once
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <wrl.h>
#include "Mesh.h"

class Pipeline;

struct ModelData {
	DirectX::XMFLOAT4X4												modelMatrix;
	DirectX::XMFLOAT4X4												modelMatrixInv;
};

class Model : public UnknownObject {
public:
	Model();
public:
	// Setup transform matrix buffer
	bool Setup(ID3D11Device*);
	// Bind transform matrix buffer and mesh list
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);

public:
	void Translate(DirectX::XMFLOAT3& dir);
	void RotateAroundOrigin(DirectX::XMFLOAT3& angle);
	ModelData GetModelData() const;
private:
	void calcModelMatrix();
	void defConstruct();
private:
	DirectX::XMFLOAT3													m_pos;
	DirectX::XMFLOAT3													m_rotateOrig;
	ModelData																m_modelData;

	Microsoft::WRL::ComPtr<ID3D11Buffer>					m_buf;
	UINT																		m_slot;
	bool																			m_hasSetup;

	bool																			m_isDirty;
};
