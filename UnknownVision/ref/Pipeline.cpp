#include "Pipeline.h"
#include "Material.h"
#include "Mesh.h"
#include "RenderTarget.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"

RenderTargetState::RenderTargetState(IRenderTarget* irt, 
	bool bc, bool ac) : rt(irt), beforeClear(bc), afterClear(ac) {}

float clearColor[] = { 0.0, 0.0, 0.0, 0.0 };

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Pipeline   //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

Pipeline::Pipeline() {} 

///////////////////
// public function
///////////////////
void Pipeline::AddRenderTarget(IRenderTarget* rt, bool bc, bool ac) {
	// 检查是否已经存在
	for (auto iter = m_interfaceRTs.begin(), end = m_interfaceRTs.end(); iter != end; ++iter) {
		if ((*iter).rt == rt) {
			(*iter).beforeClear = bc;
			(*iter).afterClear = ac;
			return;
		}
	}
	m_interfaceRTs.push_back({ rt, bc, ac });
	m_RTs.push_back(rt->GetRTV());
}

void Pipeline::RemoveRenderTarget(IRenderTarget* rt) {
	//for (auto iter = m_interfaceRTs.begin(), end = m_interfaceRTs.end(); iter != end; ++iter) {
	//	if ((*iter).rt == rt) {
	//		m_interfaceRTs.erase(iter);
	//		return;
	//	}
	//}
	for (int i = 0; i < m_interfaceRTs.size(); ++i) {
		if (m_interfaceRTs[i].rt->GetRTV() == rt->GetRTV()) {
			// 当前确实全在对应的rt
			auto iter1 = m_interfaceRTs.begin() + i;
			auto iter2 = m_RTs.begin() + i;
			m_interfaceRTs.erase(iter1);
			m_RTs.erase(iter2);
			return;
		}
	}
}

void Pipeline::SetDepthStencilView(ID3D11DepthStencilView* dsv, bool bc, bool ac) {
	m_DS = dsv;
	m_bcDS = bc;
	m_acDS = ac;
}

bool Pipeline::Setup(ID3D11Device* dev) {
	//for (auto iter = m_interfaceRTs.begin(), end = m_interfaceRTs.end(); iter != end; ++iter) {
	//	m_RTs.push_back((*iter).rt->GetRTV());
	//}
	return true;
}

void Pipeline::Bind(ID3D11DeviceContext* devCtx) {
	// bind material (bind failed if no material!)
	if (!m_material) return;
	m_material->Bind(devCtx);
	if (m_camera) m_camera->Bind(devCtx);
	// clear render target
	for (auto iter = m_interfaceRTs.begin(), end = m_interfaceRTs.end(); iter != end; ++iter) {
		if (iter->beforeClear)
			devCtx->ClearRenderTargetView(iter->rt->GetRTV(), clearColor);
	}
	if (m_DS && m_bcDS)
		devCtx->ClearDepthStencilView(m_DS, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
	devCtx->OMSetRenderTargets(m_RTs.size(), &m_RTs[0], m_DS);
	// bind meshes
	for (auto iter = m_mesh.begin(), end = m_mesh.end(); iter != end; ++iter) {
		(*iter)->Bind(devCtx);
	}
}

void Pipeline::Unbind(ID3D11DeviceContext* devCtx) {
	// Unbind Material (unbind failed if no material)
	if (!m_material) return;
	m_material->Unbind(devCtx);
	if (m_camera) m_camera->Unbind(devCtx);
	// unbind meshes
	for (auto iter = m_mesh.begin(), end = m_mesh.end(); iter != end; ++iter) {
		(*iter)->Unbind(devCtx);
	}
	// clear render target
	for (auto iter = m_interfaceRTs.begin(), end = m_interfaceRTs.end(); iter != end; ++iter) {
		if (iter->afterClear)
			devCtx->ClearRenderTargetView(iter->rt->GetRTV(), clearColor);
	}
	if (m_DS && m_acDS)
		devCtx->ClearDepthStencilView(m_DS, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
	devCtx->OMSetRenderTargets(0, NULL, NULL);
}

bool Pipeline::SetMaterial(Material* mat) {
	// 输入NULL 删除当前绑定的材质
	m_material = mat;
	return true;
}

void Pipeline::AddMesh(Mesh* mesh) {
	for (auto iter = m_mesh.begin(), end = m_mesh.end(); iter != end; ++iter) {
		if ((*iter) == mesh) return;
	}
	m_mesh.push_back(mesh);
}

void Pipeline::RemoveMesh(Mesh* mesh) {
	for (auto iter = m_mesh.begin(), end = m_mesh.end(); iter != end; ++iter) {
		if ((*iter) == mesh) m_mesh.erase(iter);
	}
}

void Pipeline::SetCamera(Camera* c) {
	m_camera = c;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   ComputePipeline   /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

CSTextureState::CSTextureState(ITexture* tex, UINT slot) : tex(tex), slot(slot) {}

CSUAState::CSUAState(IUATarget* uat, UINT slot) : uat(uat), slot(slot) {}

ComputePipeline::ComputePipeline(ComputeShader1* cs, DirectX::XMUINT3 group)
	: m_computeShader(cs), m_group(group) {}

bool ComputePipeline::Setup(ID3D11Device*) {
	//Empty
	return true;
}

void ComputePipeline::BindResource(ITexture* tex, UINT slot) {
	// 暂时没有进行绑定有效性校验
	m_texStates.push_back(CSTextureState(tex, slot));
}

void ComputePipeline::BindUATarget(IUATarget* uat, UINT slot) {
	// 暂时没有进行绑定有效性校验
	m_uaStates.push_back(CSUAState(uat, slot));
}

void ComputePipeline::Bind(ID3D11DeviceContext* devCtx) {
	m_computeShader->Bind(devCtx);
	// bind textures
	for (auto iter = m_texStates.begin(), end = m_texStates.end(); iter != end; ++iter) {
		devCtx->CSSetShaderResources(iter->slot, 1, iter->tex->GetSRV());
	}
	// bind Unorder access target
	for (auto iter = m_uaStates.begin(), end = m_uaStates.end(); iter != end; ++iter) {
		devCtx->CSSetUnorderedAccessViews(iter->slot, 1, iter->uat->GetUAV(), NULL);
	}
	devCtx->Dispatch(m_group.x, m_group.y, m_group.z);
}

void ComputePipeline::Unbind(ID3D11DeviceContext* devCtx) {
	static ID3D11ShaderResourceView* NullPtr1[] = { NULL };
	static ID3D11UnorderedAccessView* NullPtr2[] = { NULL };
	m_computeShader->Unbind(devCtx);
	// unbind textures
	for (auto iter = m_texStates.begin(), end = m_texStates.end(); iter != end; ++iter) {
		devCtx->CSSetShaderResources(iter->slot, 1, NullPtr1);
	}
	// unbind unorder access target
	for (auto iter = m_uaStates.begin(), end = m_uaStates.end(); iter != end; ++iter) {
		devCtx->CSSetUnorderedAccessViews(iter->slot, 1, NullPtr2, NULL);
	}
}
