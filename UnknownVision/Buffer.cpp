#include "Buffer.h"
#include "InfoLog.h"

ConstantBuffer::ConstantBuffer(ConstBufferDesc* desc) {
	m_isDirty = true;
	m_isBinding = false;
	m_size = 0;
	if (desc != NULL) {
		m_desc = *desc;
		init();
	} else {
		m_isInitialized = false;
		MLOG(LW, "ConstantBuffer is not initialize!");
	}
}

///////////////////
// public function
///////////////////

bool ConstantBuffer::IsUsable() const { return m_isInitialized; }

bool ConstantBuffer::Setup(ID3D11Device* dev) {
	if (!m_isInitialized) {
		MLOG(LL, "ConstantBuffer::Setup: is not initialize! setup failed!");
		return false;
	}
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&desc, sizeof(desc));
	ZeroMemory(&initData, sizeof(initData));
	// ´´½¨»º³åÇø
	m_data = std::shared_ptr<byte>(new byte[m_size]);

	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.ByteWidth = m_size;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DYNAMIC;

	initData.pSysMem = m_data.get();
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	if (FAILED(dev->CreateBuffer(&desc, &initData, m_buffer.ReleaseAndGetAddressOf()))) {
		MLOG(LL, "ConstantBuffer::Setup: can not create constant buffer!");
		return false;
	}
	return true;
}

void ConstantBuffer::Bind(ID3D11DeviceContext* devCtx) {
	if (m_isBinding) return;
	if (m_isDirty) {
		D3D11_MAPPED_SUBRESOURCE map;
		if (FAILED(devCtx->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) {
			MLOG(LL, "ConstantBuffer::Bind: Update buffer data failed!");
		}
		else {
			memcpy_s(map.pData, m_size, m_data.get(), m_size);
			devCtx->Unmap(m_buffer.Get(), 0);
			m_isDirty = false;
		}
	}
	switch (m_target) {
	case SBT_VERTEX_SHADER:
		devCtx->VSSetConstantBuffers(m_slot, 1, m_buffer.GetAddressOf());
		break;
	case SBT_PIXEL_SHADER:
		devCtx->PSSetConstantBuffers(m_slot, 1, m_buffer.GetAddressOf());
		break;
	case SBT_GEOMETRY_SHADER:
		devCtx->GSSetConstantBuffers(m_slot, 1, m_buffer.GetAddressOf());
		break;
	}
	m_isBinding = true;
}

void ConstantBuffer::Unbind(ID3D11DeviceContext* devCtx) {
	if (!m_isBinding) return;
	m_isBinding = false;
	// do not thing!
}

void ConstantBuffer::SetSlot(UINT slot) {
	if (m_isBinding) {
		MLOG(LW, "ConstantBuffer::SetSlot: buffer is now bindding! Set slot failed!");
		return;
	}
	m_slot = slot;
}

void ConstantBuffer::SetTarget(ShaderBindTarget cbbt) {
	if (m_isBinding) {
		MLOG(LW, "ConstantBuffer::SetTarget: buffer is now bindding! Set Target failed!");
		return;
	}
	m_target = cbbt;
}

ConstBufferDesc* ConstantBuffer::GetDescription() {
	return &m_desc;
}

void ConstantBuffer::Update(std::string name, void* data, SIZE_T size) {
	static const char* funcTag = "ConstantBuffer::Update: ";
	auto re = m_checkMap.find(name);
	if (re == m_checkMap.end()) {
		MLOG(LL, funcTag, LW, "There is no such variable!");
		return;
	}
	BufferVariableDesc* var = re->second;
	if (var->size != size) {
		MLOG(LW, funcTag, LW, "Size not match!");
		return;
	}
	memcpy_s(m_data.get() + var->offset, var->size, data, var->size);
	m_isDirty = true;
}

void ConstantBuffer::Update(void* data, SIZE_T size, SIZE_T offset) {
	static const char* funcTag = "ConstantBuffer::Update: ";
	if (offset + size > m_size) {
		MLOG(LW, funcTag, LW, "buffer overfloat! Update failed!");
		return;
	}
	memcpy_s(m_data.get() + offset, size, data, size);
	m_isDirty = true;
}

///////////////////
// private function
///////////////////

void ConstantBuffer::init() {
	for (auto iter = m_desc.variables.begin(), end = m_desc.variables.end();
		iter != end; ++iter) {
		if (!m_checkMap.insert(std::make_pair(iter->name.c_str(), iter._Ptr)).second) {
			MLOG(LL, "ConstantBuffer::init: insert variable ", LE, iter->name, LL, " failed!");
		}
		m_size += iter->size;
	}
	m_isInitialized = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   StructuredBuffer   /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

StructuredBuffer::StructuredBuffer(SIZE_T numEle, SIZE_T eleSize, bool isReadOnly, void* data)
	: totalSizeInByte(eleSize * numEle), eleSizeInByte(eleSize), numEle(numEle), m_data(data), isReadOnly(isReadOnly),
	slot(0), bindTarget(){}

bool StructuredBuffer::Setup(ID3D11Device* dev) {
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA subData;
	ZeroMemory(&desc, sizeof(desc));
	ZeroMemory(&subData, sizeof(subData));
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	if (!isReadOnly) desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	desc.ByteWidth = totalSizeInByte;
	desc.CPUAccessFlags = 0;
	//desc.MiscFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = eleSizeInByte;
	desc.Usage = D3D11_USAGE_DEFAULT;

	if (FAILED(dev->CreateBuffer(&desc, NULL, m_buffer.ReleaseAndGetAddressOf()))) {
		MLOG(LW, "StructuredBuffer::Setup: create buffer failed!");
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	viewDesc.Buffer.ElementOffset = 0;
	viewDesc.Buffer.ElementWidth = eleSizeInByte;
	viewDesc.Buffer.NumElements = numEle;
	viewDesc.Buffer.FirstElement = 0;
	if (FAILED(dev->CreateShaderResourceView(m_buffer.Get(), &viewDesc, m_srv.ReleaseAndGetAddressOf()))) {
		MLOG(LW, "StructuredBuffer::Setup: create shader Resource View failed!");
		return false;
	}

	if (!isReadOnly) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0;
		uavDesc.Buffer.NumElements = numEle;
		if (FAILED(dev->CreateUnorderedAccessView(m_buffer.Get(), &uavDesc, m_uav.ReleaseAndGetAddressOf()))) {
			MLOG(LW, "StructuredBuffer::Setup: create unordered access view failed!");
			return false;
		}
	}
	return true;
}

//void StructuredBuffer::Bind(ID3D11DeviceContext* devCtx) {
//	switch (bindTarget) {
//	case SBT_VERTEX_SHADER:
//		devCtx->VSSetShaderResources(slot, 1, m_srv.GetAddressOf());
//		break;
//	case SBT_PIXEL_SHADER:
//		devCtx->PSSetShaderResources(slot, 1, m_srv.GetAddressOf());
//		break;
//	case SBT_GEOMETRY_SHADER:
//		devCtx->GSSetShaderResources(slot, 1, m_srv.GetAddressOf());
//		break;
//	case SBT_COMPUTE_SHADER:
//		if (isReadOnly) devCtx->CSSetShaderResources(slot, 1, m_srv.GetAddressOf());
//		else devCtx->CSSetUnorderedAccessViews(slot, 1, m_uav.GetAddressOf(), NULL);
//		break;
//	default:
//		MLOG(LE, "StructuedBuffer::Bind: invalid binding point!");
//		break;
//	}
//}
//
//void StructuredBuffer::Unbind(ID3D11DeviceContext* devCtx) {
//	static ID3D11ShaderResourceView* NULLPointer1= { NULL };
//	static ID3D11UnorderedAccessView* NULLPointer2 = { NULL };
//	switch (bindTarget) {
//	case SBT_VERTEX_SHADER:
//		devCtx->VSSetShaderResources(slot, 1, &NULLPointer1);
//		break;
//	case SBT_PIXEL_SHADER:
//		devCtx->PSSetShaderResources(slot, 1, &NULLPointer1);
//		break;
//	case SBT_GEOMETRY_SHADER:
//		devCtx->GSSetShaderResources(slot, 1, &NULLPointer1);
//		break;
//	case SBT_COMPUTE_SHADER:
//		if (isReadOnly) devCtx->CSSetShaderResources(slot, 1, &NULLPointer1);
//		else devCtx->CSSetUnorderedAccessViews(slot, 1, &NULLPointer2, NULL);
//		break;
//	default:
//		MLOG(LE, "StructuedBuffer::Unbind: invalid binding point!");
//		break;
//	}
//}


ID3D11UnorderedAccessView** StructuredBuffer::GetUAV() {
	if (isReadOnly) return nullptr;
	else return m_uav.GetAddressOf();
}

ID3D11ShaderResourceView** StructuredBuffer::GetSRV() {
	return m_srv.GetAddressOf();
}
