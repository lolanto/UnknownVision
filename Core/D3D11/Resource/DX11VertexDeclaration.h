#ifndef D3D11_VERTEX_DECLARATION
#define D3D11_VERTEX_DECLARATION

#include "../DX11_UVConfig.h"

namespace UnknownVision {
	class DX11_VertexDeclaration : public VertexDeclaration {
	public:
		DX11_VertexDeclaration(SmartPTR<ID3D11InputLayout>& vd, uint32_t RID)
			: VertexDeclaration(RID), m_inputLayout(vd) {}
		ID3D11InputLayout* GetDeclaration() { return m_inputLayout.Get(); }
	private:
		SmartPTR<ID3D11InputLayout> m_inputLayout;
	};
}

#endif // D3D11_VERTEX_DECLARATION