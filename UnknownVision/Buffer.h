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
	// 从某个位置开始更新缓冲区
	void Update(void* data, SIZE_T size, SIZE_T offset = 0);
	void SetSlot(UINT);
	void SetTarget(ShaderBindTarget);
	ConstBufferDesc* GetDescription();
private:
	// 初始化checkMap
	void init();
private:
	bool																				m_isInitialized;

	ConstBufferDesc															m_desc;
	std::map<std::string, BufferVariableDesc*>					m_checkMap;
	std::shared_ptr<byte>													m_data;
	Microsoft::WRL::ComPtr<ID3D11Buffer>						m_buffer;
	// 是否做过修改
	bool																				m_isDirty;
	// 是否正在绑定中
	bool																				m_isBinding;
	// constant buffer的整体大小
	SIZE_T																			m_size;
	// 绑定的slot
	UINT																			m_slot;
	// 绑定的PipeLine阶段
	ShaderBindTarget															m_target;
};

class StructuredBuffer: public IUATarget, public ITexture {
public:
	StructuredBuffer(SIZE_T numEle, SIZE_T eleSize, bool isReadOnly = true, void* data = NULL);
	bool Setup(ID3D11Device*);

	ID3D11UnorderedAccessView** GetUAV();
	ID3D11ShaderResourceView** GetSRV();

public:
	// 缓冲区的大小(字节为单位)
	const SIZE_T																		totalSizeInByte;
	// 元素的大小
	const SIZE_T																		eleSizeInByte;
	// 缓冲区中的元素个数
	const SIZE_T																		numEle;
	// 当前缓冲区是否可写--false, 该缓冲区应该为RWStructuredBuffer
	const bool																		isReadOnly;

	UINT																				slot;
	ShaderBindTarget																bindTarget;
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer>							m_buffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_srv;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>m_uav;
	void*																				m_data;
};
