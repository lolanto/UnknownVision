
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "Utility/GeneralCammera/GeneralCamera.h"
#include "RenderSystem/RenderBackend.h"
#include "RenderSystem/RenderDevice.h"
#include "Image/Image.h"
#include "RenderSystem/BindingBoard.h"
#include "Shader/SampleShaderWithTexture.h"
#include "IMGUI_IMPL/imgui_impl_uv_glfw.h"
#include "IMGUI_IMPL/imgui_impl_uv.h"
#include <iostream>
#include <random>
#include <stack>
#include <iostream>

using namespace UnknownVision;

constexpr uint32_t gWidth = 1280	;
constexpr uint32_t gHeight = 800;

const float BLUE[4] = { 0.2f, 0.4f, 0.8f, 1.0f };

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main() {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	Image::Init();
	GLFWwindow* window = nullptr;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		return 0;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(gWidth, gHeight, "test", nullptr, nullptr);
	if (window == nullptr) {
		glfwTerminate();
		return 0;
	}
	auto backend = RenderBackend::Get();
	DX12BackendUsedData bkData = { reinterpret_cast<size_t>(glfwGetWin32Window(window)), gWidth, gHeight };
	assert(backend->Initialize());
	auto* pDevice = backend->CreateDevice(&bkData);
	assert(pDevice->Initialize(""));

	CommandUnit* cmdUnit = pDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);
	assert(ImGui_ImplUVGlfw_Init(window, true));
	assert(ImGui_ImplUV_Init(pDevice, backend, NUMBER_OF_BACK_BUFFERS));
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	while (!glfwWindowShouldClose(window))
	{
		GPUResource* rts[] = { pDevice->BackBuffer() };
		ImGui_ImplUVGlfw_NewFrame();
		ImGui_ImplUV_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}
		ImGui::Render();
		cmdUnit->TransferState(pDevice->BackBuffer(), RESOURCE_STATE_RENDER_TARGET);
		cmdUnit->ClearRenderTarget(pDevice->BackBuffer(), BLUE);
		cmdUnit->BindRenderTargets(rts, 1, nullptr);
		ImGui_ImplUV_RenderDrawData(ImGui::GetDrawData(), cmdUnit);
		cmdUnit->TransferState(pDevice->BackBuffer(), RESOURCE_STATE_PRESENT);
		size_t fenceValue = cmdUnit->Flush(false);
		ImGui_ImplUV_FrameEnd(fenceValue);
		pDevice->Present();
		pDevice->UpdatePerFrame();
		pDevice->FreeCommandUnit(&cmdUnit);
		cmdUnit = pDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);
		glfwPollEvents();
	}

	ImGui_ImplUV_Shutdown(pDevice);
	ImGui_ImplUVGlfw_Shutdown();
	ImGui::DestroyContext();
	Image::Shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}