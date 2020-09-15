#pragma once
#include "../UVConfig.h"
#include "../UVType.h"

#include <imgui.h>

BEG_NAME_SPACE
class CommandUnit;
class RenderDevice;
class RenderBackend;
IMGUI_IMPL_API bool ImGui_ImplUV_Init(RenderDevice* dev, RenderBackend* backend, size_t reserveFrames);
IMGUI_IMPL_API void ImGui_ImplUV_Shutdown(RenderDevice* dev);
IMGUI_IMPL_API void ImGui_ImplUV_NewFrame();
/** 要求backBuffer必须处在RenderTarget的状态 */
IMGUI_IMPL_API void ImGui_ImplUV_RenderDrawData(ImDrawData* drawData, CommandUnit* cmdUnit);
IMGUI_IMPL_API void ImGui_ImplUV_FrameEnd(size_t fenceValue);

END_NAME_SPACE
