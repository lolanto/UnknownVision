#include "DX11ResMgr.h"
#include <fstream>
#include <d3dcompiler.h>

namespace UnknownVision {
	int DX11_ShaderMgr::CreateShaderFromBinaryFile(ShaderType type, const char* fileName) {
		SmartPTR<ID3DBlob> byteCodePtr;
		SmartPTR<ID3D11DeviceChild> shaderPtr;
		SmartPTR<ID3D11VertexShader> vs;
		SmartPTR<ID3D11PixelShader> ps;
		std::ifstream file(fileName, std::ios::in | std::ios::binary);
		if (!file.is_open()) {
			return -1;
		}
		file.seekg(0, std::ios::end);
		size_t fileSize = static_cast<size_t>(file.tellg());
		file.seekg(0, std::ios::beg);
		if (FAILED(D3DCreateBlob(fileSize, byteCodePtr.GetAddressOf()))) {
			file.close();
			return -1;
		}
		file.read(reinterpret_cast<char*>(byteCodePtr->GetBufferPointer()), fileSize);
		file.seekg(0, std::ios::beg);
		std::vector<uint8_t> byteCodes(fileSize);
		file.read(reinterpret_cast<char*>(byteCodes.data()), fileSize);
		file.close();
		switch (type) {
		case ST_Vertex_Shader:
			//if (FAILED(m_dev->CreateVertexShader(byteCodePtr->GetBufferPointer(), byteCodePtr->GetBufferSize(), nullptr, vs.GetAddressOf()))) {
			if (FAILED(m_dev->CreateVertexShader(byteCodes.data(), fileSize, nullptr, vs.GetAddressOf()))) {
				return -1;
			}
			shaderPtr = vs;
			break;
		case ST_Pixel_Shader:
			if (FAILED(m_dev->CreatePixelShader(byteCodePtr->GetBufferPointer(), fileSize, nullptr, ps.GetAddressOf()))) {
				return -1;
			}
			shaderPtr = ps;
			break;
		default:
			assert(false);
		}
		
		m_shaders.push_back(DX11_Shader(type, shaderPtr, 0));
		// vertex shader需要保留编译后的代码，用来生成input layout
		if (m_shaders.back().Type == ST_Vertex_Shader) {
			m_shaders.back().SetByteCode(byteCodePtr);
		}
		return m_shaders.size() - 1;
	}
}
