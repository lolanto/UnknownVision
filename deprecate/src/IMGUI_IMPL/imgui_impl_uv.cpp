#include "imgui_impl_uv.h"
#include "imgui_impl_uv_shader.hpp"
#include "../RenderSystem/RenderDevice.h"
#include "../RenderSystem/BindingBoard.h"
#include "../RenderSystem/RenderBackend.h"
#include "../GPUResource/GPUResource.h"
#include "../Image/Image.h"
#include <memory>
#include <vector>

BEG_NAME_SPACE

GraphicsPipelineObject* gPSO;
std::unique_ptr<VertexShader> gvs;
std::unique_ptr<PixelShader> gps;
std::unique_ptr<BindingBoard> gBindingPS;
std::unique_ptr<Texture2D> gFontTexture;
struct FrameResource {
	std::unique_ptr<Buffer> vtxBuffer;
	std::unique_ptr<Buffer> idxBuffer;
	std::unique_ptr<BindingBoard> vsBindingBoard;
	std::unique_ptr<Buffer> vsBuffer;
	size_t fenceValue = SIZE_MAX;
};
size_t gNumFrames; /**< 缓冲的帧数量 */
size_t gCurrentFrameIdx;
std::vector<FrameResource> gFrameResources;

RasterizeOptions IMGUI_RASTERIZE_FUNC() {
	RasterizeOptions options;
	options.counterClockWiseIsFront = false;
	options.cullMode = CULL_MODE_NONE;
	options.fillMode = FILL_MODE_SOLID;
	options.primitive = PRIMITIVE_TYPE_TRIANGLE_LIST;
	return options;
}

OutputStageOptions IMGUI_OUTPUT_STAGE_FUNC() {
	OutputStageOptions options;
	options.rtvFormats[0] = ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM;
	options.dsvFormat = ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT;
	options.enableDepthTest = false;
	options.blendingSettings[0].enable = true;
	options.blendingSettings[0].srcBlend = BLEND_OPTION_SRC_ALPHA;
	options.blendingSettings[0].destBlend = BLEND_OPTION_INV_SRC_ALPHA;
	options.blendingSettings[0].blendOp = BLEND_OPERATION_ADD;
	options.blendingSettings[0].srcBlendAlpha = BLEND_OPTION_INV_SRC_ALPHA;
	options.blendingSettings[0].destBlendAlpha = BLEND_OPTION_ZERO;
	options.blendingSettings[0].blendOpAlpha = BLEND_OPERATION_ADD;
	return options;
}


void ImGui_ImplUV_CreateFontsTexture(RenderDevice* pDev) {
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixelData = nullptr;
	int width = 0, height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixelData, &width, &height); /**< RGBA32指RGBA加起来一共32位 */
	std::unique_ptr<Image> fontTex = Image::LoadImageFromMemory(pixelData, width, height, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM);
	/** 向GPU上传纹理资源 */
	gFontTexture.reset(pDev->CreateTexture2D(width, height, 1, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM, ResourceStatus(RESOURCE_USAGE_SHADER_RESOURCE, RESOURCE_FLAG_STABLY)));
	assert(gFontTexture != nullptr);
	auto cmdUnit = pDev->RequestCommandUnit(DEFAULT_COMMAND_UNIT);
	assert(pDev->WriteToTexture2D(fontTex.get(), gFontTexture.get(), cmdUnit));
	cmdUnit->TransferState(gFontTexture.get(), RESOURCE_STATE_SHADER_RESOURCE);
	cmdUnit->Flush(true);
	pDev->FreeCommandUnit(&cmdUnit);
}

IMGUI_IMPL_API bool ImGui_ImplUV_Init(RenderDevice* pDev, RenderBackend* pBackend, size_t reserveFrames) {
	gvs = std::make_unique<IMGUI_VERTEX_SHADER>();
	gps = std::make_unique<IMGUI_PIXEL_SHADER>();
	assert(pBackend->InitializeShaderObject(gvs.get()));
	assert(pBackend->InitializeShaderObject(gps.get()));
	gPSO = (pDev->BuildGraphicsPipelineObject(gvs.get(), gps.get(), IMGUI_RASTERIZE_FUNC, IMGUI_OUTPUT_STAGE_FUNC, IMGUI_VERTEX_SHADER::GetVertexAttributes));
	assert(gPSO != nullptr);
	ImGui_ImplUV_CreateFontsTexture(pDev);

	gBindingPS.reset(pDev->RequestBindingBoard(1, DEFAULT_COMMAND_UNIT));
	assert(gBindingPS != nullptr);
	gBindingPS->BindingResource(0, gFontTexture.get(), SHADER_PARAMETER_TYPE_TEXTURE_R);
	gBindingPS->Close();

	gFrameResources.resize(reserveFrames);
	gNumFrames = reserveFrames;
	gCurrentFrameIdx = 0;

	return true;
}

IMGUI_IMPL_API void ImGui_ImplUV_SetupRenderState(ImDrawData* drawData, CommandUnit* cmdUnit, RenderDevice* pDev) {
	auto& curFrameResource = gFrameResources[gCurrentFrameIdx];
	/** 拷贝自imgui dx12 binding关于投影矩阵的计算方式 */
	{
		float L = drawData->DisplayPos.x;
		float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
		float T = drawData->DisplayPos.y;
		float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
		float mvp[16] =
		{
			2.0f / (R - L),   0.0f,           0.0f,       0.0f,
			0.0f,         2.0f / (T - B),     0.0f,       0.0f,
			0.0f,         0.0f,           0.5f,       0.0f,
			(R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f
		};
		pDev->WriteToBuffer(mvp, curFrameResource.vsBuffer.get(), sizeof(mvp), 0, cmdUnit);
	}

	ViewPort vp;
	vp.width = drawData->DisplaySize.x;
	vp.height = drawData->DisplaySize.y;
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;
	vp.topLeftX = vp.topLeftY = 0.0f;

	cmdUnit->BindViewports(1, &vp);
	Buffer* vtxBuffers[] = { curFrameResource.vtxBuffer.get() };
	cmdUnit->BindVertexBuffers(0, 1, vtxBuffers);
	cmdUnit->BindIndexBuffer(curFrameResource.idxBuffer.get());
	cmdUnit->SetBindingBoard(0, curFrameResource.vsBindingBoard.get());
	cmdUnit->SetBindingBoard(1, gBindingPS.get());
	cmdUnit->BindPipeline(gPSO);
}

IMGUI_IMPL_API void ImGui_ImplUV_RenderDrawData(ImDrawData* drawData, CommandUnit* cmdUnit)
{
	auto& curFrameResource = gFrameResources[gCurrentFrameIdx];
	RenderDevice* pDev = cmdUnit->GetDevice();
	if (curFrameResource.fenceValue != SIZE_MAX) {
		cmdUnit->GetDevice()->WaitForCommandExecutionComplete(curFrameResource.fenceValue, DEFAULT_COMMAND_UNIT);
	}
	if (curFrameResource.vtxBuffer == nullptr || curFrameResource.vtxBuffer->Capacity() < drawData->TotalVtxCount) {
		/** 重新申请一个更大的顶点缓冲以容纳更多顶点 */
		curFrameResource.vtxBuffer.reset(pDev->CreateBuffer(
			drawData->TotalIdxCount + 5000, sizeof(ImDrawVert), ResourceStatus(RESOURCE_USAGE_VERTEX_BUFFER, RESOURCE_FLAG_FREQUENTLY)));
	}
	if (curFrameResource.idxBuffer == nullptr || curFrameResource.idxBuffer->Capacity() < drawData->TotalIdxCount) {
		/** 重新申请一个更大的索引缓冲以容纳更多索引数据 */
		curFrameResource.idxBuffer.reset(pDev->CreateBuffer(
			drawData->TotalIdxCount + 5000, sizeof(ImDrawIdx), ResourceStatus(RESOURCE_USAGE_INDEX_BUFFER, RESOURCE_FLAG_FREQUENTLY)));
	}
	if (curFrameResource.vsBuffer == nullptr) {
		/** 申请Vertex Shader常量缓冲，存储投影变换矩阵，同时还需要申请BindingBoard */
		curFrameResource.vsBuffer.reset(pDev->CreateBuffer(
			16, sizeof(float), ResourceStatus(RESOURCE_USAGE_CONSTANT_BUFFER, RESOURCE_FLAG_FREQUENTLY)));
		curFrameResource.vsBindingBoard.reset(cmdUnit->GetDevice()->RequestBindingBoard(1, DEFAULT_COMMAND_UNIT));
		curFrameResource.vsBindingBoard->BindingResource(0, curFrameResource.vsBuffer.get(), SHADER_PARAMETER_TYPE_BUFFER_R);
		curFrameResource.vsBindingBoard->Close();
	}
	size_t vtxOffset = 0, idxOffset = 0;
	for (int i = 0; i < drawData->CmdListsCount; ++i) {
		const ImDrawList* cmd_list = drawData->CmdLists[i];
		pDev->WriteToBuffer(cmd_list->VtxBuffer.Data, curFrameResource.vtxBuffer.get(), cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), vtxOffset, cmdUnit);
		pDev->WriteToBuffer(cmd_list->IdxBuffer.Data, curFrameResource.idxBuffer.get(), cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), idxOffset, cmdUnit);
		vtxOffset += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
		idxOffset += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
	}
	ImGui_ImplUV_SetupRenderState(drawData, cmdUnit, pDev);
	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int global_vtx_offset = 0;
	int global_idx_offset = 0;
	ImVec2 clip_off = drawData->DisplayPos;
	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = drawData->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != NULL)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					ImGui_ImplUV_SetupRenderState(drawData, cmdUnit, pDev);
				else
					pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				// Apply Scissor, Bind texture, Draw
				ScissorRect sr;
				sr.left = pcmd->ClipRect.x - clip_off.x;
				sr.top = pcmd->ClipRect.y - clip_off.y;
				sr.right = pcmd->ClipRect.z - clip_off.x;
				sr.bottom = pcmd->ClipRect.w - clip_off.y;
				cmdUnit->BindScissorRects(1, &sr);
				cmdUnit->Draw(pcmd->IdxOffset + global_idx_offset, pcmd->ElemCount, pcmd->VtxOffset + global_vtx_offset);
			}
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}
}

IMGUI_IMPL_API void ImGui_ImplUV_NewFrame()
{
	gCurrentFrameIdx = (gCurrentFrameIdx + 1) % gNumFrames;
	return;
}

IMGUI_IMPL_API void ImGui_ImplUV_Shutdown(RenderDevice* pDev)
{
	/** 等待所有帧运行结束 */
	for (auto& frameRes : gFrameResources) {
		if (frameRes.fenceValue != SIZE_MAX) {
			pDev->WaitForCommandExecutionComplete(frameRes.fenceValue, DEFAULT_COMMAND_UNIT);
			if (frameRes.idxBuffer) frameRes.idxBuffer->Release();
			if (frameRes.vtxBuffer) frameRes.vtxBuffer->Release();
			if (frameRes.vsBuffer) frameRes.vsBuffer->Release();
			if (frameRes.vsBindingBoard) frameRes.vsBindingBoard->Reset();
		}
	}
	gvs.reset(nullptr);
	gps.reset(nullptr);
	gFontTexture->Release();
	gFontTexture.reset(nullptr);
	gBindingPS->Reset();
}

IMGUI_IMPL_API void ImGui_ImplUV_FrameEnd(size_t fenceValue)
{
	gFrameResources[gCurrentFrameIdx].fenceValue = fenceValue;
}

END_NAME_SPACE
