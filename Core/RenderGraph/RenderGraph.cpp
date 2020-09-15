#include "../RenderSystem/RenderDevice.h"
#include "RenderGraph.h"
#include <assert.h>

BEG_NAME_SPACE

bool RenderGraph::Setup(RenderDevice* device) {
	m_device = device;
	/** 配置默认的绑定接口 */
	for (auto& rp : m_passes)
		rp->Setup(this);
}

void RenderGraph::Execute() {
	for (auto rp : m_executeOrder) {
		// 资源装配
		rp->Exec(this);
	}
}

bool RenderGraph::Compile() {
	// 筛选有效pass
	// 假设m_passes中的所有pass有效
	// 安排执行顺序
	// 假设就按照m_passes中的顺序执行
	for (auto& rp : m_passes)
		m_executeOrder.push_back(rp.get());
	// 创建资源
	
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
		assert(false); // 暂时不考虑没找到的情况
	}
	return iter->second;
}

END_NAME_SPACE
