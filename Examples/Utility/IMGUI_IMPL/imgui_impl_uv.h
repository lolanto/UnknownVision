#pragma once
#include <UVConfig.h>
#include <imgui/imgui.h>

BEG_NAME_SPACE
class CommandUnit;
class RenderDevice;
class RenderBackend;
END_NAME_SPACE

IMGUI_IMPL_API bool ImGui_ImplUV_Init(UnknownVision::RenderDevice* dev, UnknownVision::RenderBackend* backend, size_t reserveFrames);
IMGUI_IMPL_API void ImGui_ImplUV_Shutdown(UnknownVision::RenderDevice* dev);
IMGUI_IMPL_API void ImGui_ImplUV_NewFrame();
/** 要求backBuffer必须处在RenderTarget的状态 */
IMGUI_IMPL_API void ImGui_ImplUV_RenderDrawData(ImDrawData* drawData, UnknownVision::CommandUnit* cmdUnit);
IMGUI_IMPL_API void ImGui_ImplUV_FrameEnd(size_t fenceValue);


