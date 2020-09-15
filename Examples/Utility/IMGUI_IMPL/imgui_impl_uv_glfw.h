#pragma once
#include <imgui/imgui.h>
#include <UVConfig.h>
struct GLFWwindow;


IMGUI_IMPL_API bool     ImGui_ImplUVGlfw_Init(GLFWwindow* window, bool install_callbacks);
IMGUI_IMPL_API void     ImGui_ImplUVGlfw_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplUVGlfw_NewFrame();

// GLFW callbacks
// - When calling Init with 'install_callbacks=true': GLFW callbacks will be installed for you. They will call user's previously installed callbacks, if any.
// - When calling Init with 'install_callbacks=false': GLFW callbacks won't be installed. You will need to call those function yourself from your own GLFW callbacks.
IMGUI_IMPL_API void     ImGui_ImplUVGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
IMGUI_IMPL_API void     ImGui_ImplUVGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
IMGUI_IMPL_API void     ImGui_ImplUVGlfw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
IMGUI_IMPL_API void     ImGui_ImplUVGlfw_CharCallback(GLFWwindow* window, unsigned int c);

