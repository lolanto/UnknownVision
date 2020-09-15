#include "../RenderSystem/RenderDevice.h"
#include "RenderGraph.h"
#include <assert.h>

BEG_NAME_SPACE

bool RenderGraph::Setup(RenderDevice* device) {
	m_device = device;
	/** ����Ĭ�ϵİ󶨽ӿ� */
	for (auto& rp : m_passes)
		rp->Setup(this);
}

void RenderGraph::Execute() {
	for (auto rp : m_executeOrder) {
		// ��Դװ��
		rp->Exec(this);
	}
}

bool RenderGraph::Compile() {
	// ɸѡ��Чpass
	// ����m_passes�е�����pass��Ч
	// ����ִ��˳��
	// ����Ͱ���m_passes�е�˳��ִ��
	for (auto& rp : m_passes)
		m_executeOrder.push_back(rp.get());
	// ������Դ
	
}

BufferResourceInfo* RenderGraph::ReadFromIndexBuffer(IRenderPass* pass) {
	m_idxBuffer.ReadInPass(pass);
	return &m_idxBuffer;
}

BufferResourceInfo* RenderGraph::ReadFromVertexBuffer(IRenderPass* pass) {
	m_vtxBuffer.ReadInPass(pass);
	return &m_vtxBuffer;
}

TextureResourceInfo* RenderGraph::WriteToRenderTarget(IRenderPass* pass, const std::string& name) {
	auto& res = accessTextureResource(name);
	res.AddUsage(RESOURCE_USAGE_RENDER_TARGET);
	res.WriteInPass(pass);
	return &res;
}

TextureResourceInfo* RenderGraph::WriteToDepthStencilBuffer(IRenderPass* pass, const std::string& name) {
	auto& res = accessTextureResource(name);
	res.AddUsage(RESOURCE_USAGE_DEPTH_STENCIL);
	res.WriteInPass(pass);
	return &res;
}

bool RenderGraph::createTextureResource(TextureResourceInfo* info) {
	
}

TextureResourceInfo& RenderGraph::accessTextureResource(const std::string& name) {
	auto iter = m_textureResources.find(name);
	if (iter == m_textureResources.end()) {
		assert(false); // ��ʱ������û�ҵ������
	}
	return iter->second;
}

END_NAME_SPACE
