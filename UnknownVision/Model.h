#pragma once
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <wrl.h>
#include "Mesh.h"
#include "Buffer.h"

class Pipeline;

#define VS_MODEL_DATA_SLOT 1

struct ModelData {
	DirectX::XMFLOAT4X4												modelMatrix;
	DirectX::XMFLOAT4X4												modelMatrixInv;
};

class Model : public UnknownObject {
public:
	Model(DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f },
		DirectX::XMFLOAT3 rotate = { 0.0f, 0.0f, 0.0f });
public:
	// Setup transform matrix buffer
	bool Setup(ID3D11Device*);
	// Bind transform matrix buffer and mesh list
	void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	void Unbind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);

public:
	void Translate(DirectX::XMFLOAT3& dir);
	void RotateAroundOrigin(DirectX::XMFLOAT3& angle);
	ModelData GetModelData() const;
private:
	void calcModelMatrix();
private:
	DirectX::XMFLOAT3													m_pos;
	DirectX::XMFLOAT3													m_rotateOrig;
	bool																			m_isDirty;
	ConstantBuffer<ModelData>									m_buf;
};
