#include "DX11ResMgr.h"
#include "../DX11RenderSys.h"
#include "../../UVRoot.h"

namespace UnknownVision {
	/*
	TODO: VertexAttribute的属性转换函数，从自定义变成DX使用的
	创建Inputlayout;
	*/

	VertexDeclarationIdx DX11_VertexDeclarationMgr::CreateVertexDeclaration(const std::vector<SubVertexAttributeDesc>& verAttDescs, ShaderIdx shaderIndex) {
		ShaderMgr& sm = Root::GetInstance().GetShaderMgr();
		const DX11_Shader& shader = static_cast<const DX11_Shader&>(sm.GetShader(shaderIndex));
		ID3D11Device* dev = static_cast<DX11_RenderSys&>(Root::GetInstance().GetRenderSys()).GetDevice();
		assert(shader.Type == ST_Vertex_Shader);
		
		std::vector<D3D11_INPUT_ELEMENT_DESC> inputEleDescs(verAttDescs.size());
		auto& inputEleIter = inputEleDescs.begin();
		for (const auto& verAttIter : verAttDescs) {
			inputEleIter->SemanticName = verAttIter.semantic;
			inputEleIter->InputSlot = verAttIter.bufIdx;
			inputEleIter->SemanticIndex = verAttIter.index;
			inputEleIter->AlignedByteOffset = verAttIter.byteOffset;
			inputEleIter->InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			inputEleIter->InstanceDataStepRate = 0; // 当inputSlotClass为VertexData时，该值必须为0
			inputEleIter->Format = VertexAttributeDataType2DXGI_FORMAT(verAttIter.dataType);
			++inputEleIter;
		}
		
		assert(dev != nullptr);
		SmartPTR<ID3D11InputLayout> inputLayout;
		HRESULT hr = dev->CreateInputLayout(inputEleDescs.data(), inputEleDescs.size(), shader.ByteCode()->GetBufferPointer(),
			shader.ByteCode()->GetBufferSize(), inputLayout.ReleaseAndGetAddressOf());
		if (FAILED(hr)) {
			MLOG(LL, __FUNCTION__, LW, " Create Input Layout Failed");
			return VertexDeclarationIdx(-1);
		}
		m_vertexDecls.push_back(DX11_VertexDeclaration(inputLayout, 0));
		return VertexDeclarationIdx(m_vertexDecls.size() - 1);
	}
}
