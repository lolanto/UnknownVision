#pragma once
#include "imgui_impl_uv.h"
#include "imgui_impl_uv_glfw.h"
#include <GraphicsInterface/RenderDevice.h>
#include <GraphicsInterface/RenderBackend.h>

static void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

static GLFWwindow* setupIMGUI(int width, int height, const char* windowName) {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	GLFWwindow* window = nullptr;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		return 0;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, windowName, nullptr, nullptr);
	if (window == nullptr) {
		glfwTerminate();
		return 0;
	}
	return window;
}

static void setupIMGUI_Callback(GLFWwindow* window,
	UnknownVision::RenderDevice* pDevice,
	UnknownVision::RenderBackend* pBackend,
	GLFWmousebuttonfun mousebuttonCallback = nullptr,
	GLFWkeyfun keycallback = nullptr) {

	glfwSetKeyCallback(window, keycallback);
	glfwSetMouseButtonCallback(window, mousebuttonCallback);

	ImGui_ImplUVGlfw_Init(window, true);
	ImGui_ImplUV_Init(pDevice, pBackend, UnknownVision::NUMBER_OF_BACK_BUFFERS);

}

static void shutdownIMGUI(GLFWwindow* window, UnknownVision::RenderDevice* pDevice) {
	ImGui_ImplUV_Shutdown(pDevice);
	ImGui_ImplUVGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
}
