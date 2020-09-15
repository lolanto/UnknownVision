#include "Component.h"
#include <GraphicsInterface/RenderDevice.h>
using namespace UnknownVision;
#define USING_DXMATH

#ifdef USING_DXMATH

#include <DirectXMath.h>

class DX_TransformComponent : public ITransformComponent {
	friend class ITransformComponent;
public:
	DX_TransformComponent(IMath::IFLOAT3 position) : m_dirty(true), ITransformComponent() {
		DirectX::XMFLOAT3 pos_(position.x, position.y, position.z);
		m_position = DirectX::XMLoadFloat3(&pos_);
	}
public:
	virtual void Translate(IMath::IFLOAT3 offset) {
		m_dirty = true;
		DirectX::XMFLOAT3 offset_(offset.x, offset.y, offset.z);
		auto vecOffset = DirectX::XMLoadFloat3(&offset_);
		m_position = DirectX::XMVectorAdd(vecOffset, m_position);
	};
	virtual void RotateAround(float radiance, IMath::IFLOAT3 axis) {
		DirectX::XMFLOAT3 axis_(axis.x, std::fabs(axis.y), axis.z);
		auto vecAxis = DirectX::XMLoadFloat3(&axis_);
		auto quatRotate = DirectX::XMQuaternionRotationAxis(vecAxis, radiance / 2.0f);
		auto quatRotate_inv = DirectX::XMQuaternionRotationAxis(vecAxis, radiance / 2.0f);
		m_position = DirectX::XMQuaternionMultiply(quatRotate_inv, DirectX::XMQuaternionMultiply(m_position, quatRotate));
	}
	virtual UnknownVision::Buffer* ModelMatrix(UnknownVision::RenderDevice* ptrDevice) override final {
		if (m_modelMatrix == nullptr) {
			m_modelMatrix.reset(ptrDevice->CreateBuffer(1, sizeof(IMath::IFLOAT4X4), ResourceStatus(RESOURCE_USAGE_CONSTANT_BUFFER, RESOURCE_FLAG_FREQUENTLY)));
		}
		static IMath::IFLOAT4X4 matrixData;
		if (m_dirty) {
			auto matrix = DirectX::XMMatrixTranslationFromVector(m_position);
			DirectX::XMFLOAT4X4 output;
			DirectX::XMStoreFloat4x4(&output, matrix);
			matrixData = {
				output._11, output._12, output._13, output._14,
				output._21, output._22, output._23, output._24,
				output._31, output._32, output._33, output._34,
				output._41, output._42, output._43, output._44,
			};
			auto cmdUnit = ptrDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);
			cmdUnit->TransferState(m_modelMatrix.get(), RESOURCE_STATE_COPY_DEST);
			ptrDevice->WriteToBuffer(matrixData.data, m_modelMatrix.get(), sizeof(matrixData), 0, cmdUnit);
			cmdUnit->TransferState(m_modelMatrix.get(), RESOURCE_STATE_CONSTANT_BUFFER);
			cmdUnit->Flush(true);
			ptrDevice->FreeCommandUnit(&cmdUnit);
			m_dirty = false;
		}
		return ITransformComponent::ModelMatrix(ptrDevice);
	}
private:
	DirectX::XMVECTOR m_position;
	bool m_dirty;
};

ITransformComponent* ITransformComponent::Create(IMath::IFLOAT3 position)
{
	DX_TransformComponent* newC = new DX_TransformComponent(position);
	return newC;
}

#endif USING_DXMATH
