#include "DX11ResMgr.h"

namespace UnknownVision {

	int DX11_BufferMgr::CreateBuffer(size_t size, size_t vtxSize, uint32_t flag, uint8_t* data) {
		return -1;
	}

	int DX11_BufferMgr::CreateVertexBuffer(size_t numVtxs, size_t vtxSize, uint8_t* data) {
		if (data == nullptr) {
			return -1;
		}
		SmartPTR<ID3D11Buffer> bufPtr;
		D3D11_BUFFER_DESC bufDesc;
		D3D11_SUBRESOURCE_DATA subData;
		memset(&bufDesc, 0, sizeof(bufDesc));
		memset(&subData, 0, sizeof(subData));

		bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufDesc.ByteWidth = numVtxs * vtxSize; // size of buffer in bytes
		bufDesc.CPUAccessFlags = 0;
		bufDesc.MiscFlags = 0; // 暂时不使用特别的设置
		bufDesc.StructureByteStride = vtxSize;
		bufDesc.Usage = D3D11_USAGE_DEFAULT;

		subData.pSysMem = data;
		// 以下属性对buffer无效
		subData.SysMemPitch = 0; subData.SysMemSlicePitch = 0;

		if (FAILED(m_dev->CreateBuffer(&bufDesc, &subData, bufPtr.GetAddressOf()))) {
			assert(false);
			return -1;
		}

		m_buffers.push_back(DX11_Buffer(numVtxs * vtxSize, numVtxs,
			BF_VERTEX_BUFFER, bufPtr, 0));

		return m_buffers.size() - 1;
	}

	int DX11_BufferMgr::CreateConstantBuffer(size_t size, uint8_t* data, uint32_t flag) {
		if (data == nullptr) {
			return -1;
		}
		SmartPTR<ID3D11Buffer> bufPtr;
		D3D11_BUFFER_DESC bufDesc;
		D3D11_SUBRESOURCE_DATA subData;
		memset(&bufDesc, 0, sizeof(bufDesc));
		memset(&subData, 0, sizeof(subData));

		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.ByteWidth = size;
		bufDesc.CPUAccessFlags = (flag & BF_CPU_WRITE ? D3D11_CPU_ACCESS_WRITE : 0) |
			(flag & BF_CPU_READ ? D3D11_CPU_ACCESS_READ : 0);
		bufDesc.MiscFlags = 0; // 暂时不做特别处理
		bufDesc.StructureByteStride = 0; // 常量缓冲区暂时不管
		if (flag & BF_CPU_READ) {
			bufDesc.Usage = D3D11_USAGE_STAGING;
		}
		else if (flag & BF_CPU_WRITE) {
			bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		}
		else {
			bufDesc.Usage = D3D11_USAGE_DEFAULT;
		}

		subData.pSysMem = data;
		// 以下两个属性对缓冲区无用
		subData.SysMemPitch = subData.SysMemSlicePitch = 0;
		
		if (FAILED(m_dev->CreateBuffer(&bufDesc, &subData, bufPtr.GetAddressOf()))) {
			assert(false);
			return -1;
		}

		m_buffers.push_back(DX11_Buffer(size, 0,
			flag | BF_CONSTANT_BUFFER, bufPtr, 0));
		return m_buffers.size() - 1;
	}
}
