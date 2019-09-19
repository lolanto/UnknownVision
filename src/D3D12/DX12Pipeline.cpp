#include "DX12Pipeline.h"
#include "DX12RenderDevice.h"

BEG_NAME_SPACE

extern DX12RenderDevice* GDX12RenderDevice;
extern DX12ShaderManager GDX12ShaderManager;

void DX12PipelineManager::Build(GraphicsPipeline& input)
{
	DXCompilerHelper dxc;
	BasicShader* vs = input.vs;
	auto&& parameterPackages = vs->Pack();
	auto shaderPtr = GDX12ShaderManager[vs->GetHandle()];
	/** TODO: 消除这里的Const_cast */
	dxc.RetrieveShaderDescriptionFromByteCode(const_cast<SmartPTR<ID3DBlob>&>(shaderPtr->shaderByteCode));

}

END_NAME_SPACE
