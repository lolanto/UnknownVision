#pragma once;
#include "RenderGraph.h"

BEG_NAME_SPACE

class GPUResource;

class BasicRenderPass : public IRenderPass {
public:
	BasicRenderPass() : IRenderPass("Basic Pass") {}
	~BasicRenderPass() = default;
	ProgramType Type() const { return PROGRAM_TYPE_GRAPHICS; }
public:
	void Setup(RenderGraph* rg) override final {
		m_colorOutputs.push_back(rg->WriteToRenderTarget(this, "mainRT"));
		m_depthStencilOutput = rg->WriteToDepthStencilBuffer(this, "mainDS");
		m_vtxBuffer = rg->ReadFromVertexBuffer(this);
		m_idxBuffer = rg->ReadFromIndexBuffer(this);
	}

	void Exec(RenderGraph* rg) override final {

	}
private:
	std::vector<TextureResourceInfo*> m_colorOutputs;
	TextureResourceInfo* m_depthStencilOutput;
	BufferResourceInfo* m_vtxBuffer;
	BufferResourceInfo* m_idxBuffer;
};

END_NAME_SPACE