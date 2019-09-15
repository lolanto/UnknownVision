#pragma once
#include "DX12Config.h"
#include "../Resource/ShaderResource.h"
#include "DX12RenderBasic.h"
#include <assert.h>

BEG_NAME_SPACE

extern class DX12RenderDevice GDevice;

class DX12ConstantBufferView : public ConstantBufferView {
public:
	DX12ConstantBufferView() = default;
	void Initialize(D3D12_CONSTANT_BUFFER_VIEW_DESC desc) {
		m_desc = desc;
	}
	void Emplacement(D3D12_CPU_DESCRIPTOR_HANDLE place) {
		GDevice.GetDevice()->CreateConstantBufferView(&m_desc, place);
	}
private:
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_desc;
};

END_NAME_SPACE
