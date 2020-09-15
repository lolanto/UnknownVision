#include "DX11ResMgr.h"
#include "../DX11RenderSys.h"
#include "../../Utility/FileContainer/FileContainer.h"
#include "../../UVRoot.h"
#include <fstream>
#include <d3dcompiler.h>

namespace UnknownVision {
	ShaderIdx DX11_ShaderMgr::CreateShaderFromBinaryFile(ShaderType type, const char* fileName) {
		ID3D11Device* dev = static_cast<DX11_RenderSys&>(Root::GetInstance().GetRenderSys()).GetDevice();
		assert(dev != nullptr);
		SmartPTR<ID3DBlob> byteCodePtr;
		SmartPTR<ID3D11DeviceChild> shaderPtr;
		SmartPTR<ID3D11VertexShader> vs;
		SmartPTR<ID3D11PixelShader> ps;
		FileContainer fc(fileName, std::ios::in | std::ios::binary);
		if (!fc.IsOpen()) {
			MLOG(LE, __FUNCTION__, LL, " open binary file failed!");
			return ShaderIdx(-1);
		}
		if (FAILED(D3DCreateBlob(fc.FileSize(), byteCodePtr.GetAddressOf()))) {
			MLOG(LE, __FUNCTION__, LL, " create bytecode buffer failed!");
			return ShaderIdx(-1);
		}
		fc.ReadFile(0, fc.FileSize(), reinterpret_cast<char*>(byteCodePtr->GetBufferPointer()));
		switch (type) {
		case ST_Vertex_Shader:
			if (FAILED(dev->CreateVertexShader(byteCodePtr->GetBufferPointer(), fc.FileSize(), nullptr, vs.GetAddressOf()))) {
				return ShaderIdx(-1);
			}
			shaderPtr = vs;
			break;
		case ST_Pixel_Shader:
			if (FAILED(dev->CreatePixelShader(byteCodePtr->GetBufferPointer(), fc.FileSize(), nullptr, ps.GetAddressOf()))) {
				return ShaderIdx(-1);
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
		return ShaderIdx(m_shaders.size() - 1);
	}
}
