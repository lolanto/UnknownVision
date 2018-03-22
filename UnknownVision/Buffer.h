#pragma once

#include <memory>
#include <map>
#include <wrl.h>

#include "UnknownObject.h"
#include "RenderTarget.h"
#include "Texture.h"

class ConstantBuffer : public UnknownObject{
public:
	ConstantBuffer(ConstBufferDesc*);
public:
	bool IsUsable() const;
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);
	void Update(std::string name, void* data, SIZE_T size);
	// ��ĳ��λ�ÿ�ʼ���»�����
	void Update(void* data, SIZE_T size, SIZE_T offset = 0);
	void SetSlot(UINT);
	void SetTarget(ShaderBindTarget);
	ConstBufferDesc* GetDescription();
private:
	// ��ʼ��checkMap
	void init();
private:
	bool																				m_isInitialized;

	ConstBufferDesc															m_desc;
	std::map<std::string, BufferVariableDesc*>					m_checkMap;
	std::shared_ptr<byte>													m_data;
	Microsoft::WRL::ComPtr<ID3D11Buffer>						m_buffer;
	// �Ƿ������޸�
	bool																				m_isDirty;
	// �Ƿ����ڰ���
	bool																				m_isBinding;
	// constant buffer�������С
	SIZE_T																			m_size;
	// �󶨵�slot
	UINT																			m_slot;
	// �󶨵�PipeLine�׶�
	ShaderBindTarget															m_target;
};

class StructuredBuffer: public IUATarget, public ITexture {
public:
	StructuredBuffer(SIZE_T numEle, SIZE_T eleSize, bool isReadOnly = true, void* data = NULL);
	bool Setup(ID3D11Device*);

	ID3D11UnorderedAccessView** GetUAV();
	ID3D11ShaderResourceView** GetSRV();

public:
	// �������Ĵ�С(�ֽ�Ϊ��λ)
	const SIZE_T																		totalSizeInByte;
	// Ԫ�صĴ�С
	const SIZE_T																		eleSizeInByte;
	// �������е�Ԫ�ظ���
	const SIZE_T																		numEle;
	// ��ǰ�������Ƿ��д--false, �û�����Ӧ��ΪRWStructuredBuffer
	const bool																		isReadOnly;

	UINT																				slot;
	ShaderBindTarget																bindTarget;
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer>							m_buffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_srv;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>m_uav;
	void*																				m_data;
};
