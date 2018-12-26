#include "MeshManager.h"
#include "Mesh.h"
#include "InfoLog.h"
#include <DirectXMath.h>
#include <fstream>

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT2;
using Microsoft::WRL::ComPtr;

const char* inputLayoutShader = "../Debug/inputLayout.cso";

MeshManager::MeshManager(ID3D11Device** dev, ID3D11DeviceContext** devContext)
	: m_dev(dev), m_devContext(devContext), m_curToken(0) {
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	// temporary shader for creating input layout
	std::fstream file(inputLayoutShader, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		MLOG(LL, "open temporary shader failed! MeshManager can not initialize!");
		return;
	}
	byte* byteCode = NULL;
	size_t size = -1;
	file.seekg(0, std::ios::end);
	size = file.tellg();
	file.seekg(0, std::ios::beg);
	byteCode = new byte[size];
	file.read(reinterpret_cast<char*>(byteCode), size);
	file.close();
	// Create input layout
	HRESULT hr;
	hr = (*m_dev)->CreateInputLayout(inputElementDesc, 4, byteCode, size, m_inputLayout.ReleaseAndGetAddressOf());
	delete[] byteCode;
	if (FAILED(hr)) {
		MLOG(LL, "Create Input Layout failed! MeshManager can not initialize!");
		return;
	}
}
