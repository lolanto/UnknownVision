#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <GraphicsInterface/RenderBackend.h>
#include <GraphicsInterface/RenderDevice.h>
#include <GraphicsInterface/Pipeline.h>
#include <Utility/Image/Image.h>
#include <GraphicsInterface/BindingBoard.h>
#include <Utility/IMGUI_IMPL/imgui_impl_uv_helper.h>
#include <Utility/GeneralCamera/GeneralCamera.h>
#include <MathInterface/MathInterface.hpp>

#include <iostream>
#include <random>
#include <stack>
#include <iostream>

#include "MeshImpl_GLTF.h"
#include "LoadMeshPass.hpp"

using namespace UnknownVision;

constexpr uint32_t gWidth = 1280	;
constexpr uint32_t gHeight = 800;

const float BLUE[4] = { 0.2f, 0.4f, 0.8f, 1.0f };

struct {
	std::unique_ptr<UVCameraUtility::ICamera> camera;
	std::unique_ptr<UVCameraUtility::ICameraController> cameraController;
	glm::vec2 mouse_pos;
	RenderBackend* pBackend;
	RenderDevice* pDevice;
	GLFWwindow* pWindow;
	float deltaTime;
} GlobalData;

auto IMGUI_FRAME_FUNC = []() {
	ImGui::Begin("Controller Pad");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
};


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_W && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_W, action == GLFW_PRESS);
	if (key == GLFW_KEY_S && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_S, action == GLFW_PRESS);
	if (key == GLFW_KEY_A && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_A, action == GLFW_PRESS);
	if (key == GLFW_KEY_D && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_D, action == GLFW_PRESS);
	if (key == GLFW_KEY_Q && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_Q, action == GLFW_PRESS);
	if (key == GLFW_KEY_E && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_E, action == GLFW_PRESS);
}

static void mousebutton_callback(GLFWwindow* window, int button, int action, int mods) {
	if (GlobalData.camera != nullptr && GlobalData.cameraController != nullptr) {
		switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT:
			GlobalData.cameraController->MouseCallback((float)GlobalData.mouse_pos.x, (float)GlobalData.mouse_pos.y, UVCameraUtility::MOUSE_BUTTON_RIGHT, action == GLFW_PRESS);
			break;
		case GLFW_MOUSE_BUTTON_LEFT:
			GlobalData.cameraController->MouseCallback((float)GlobalData.mouse_pos.x, (float)GlobalData.mouse_pos.y, UVCameraUtility::MOUSE_BUTTON_LEFT, action == GLFW_PRESS);
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			GlobalData.cameraController->MouseCallback((float)GlobalData.mouse_pos.x, (float)GlobalData.mouse_pos.y, UVCameraUtility::MOUSE_BUTTON_MID, action == GLFW_PRESS);
			break;
		}
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (GlobalData.camera != nullptr && GlobalData.cameraController != nullptr) {
		GlobalData.mouse_pos = { xpos, ypos };
		GlobalData.cameraController->MouseCallback((float)xpos, (float)ypos, UVCameraUtility::MOUSE_BUTTON_NONE, false);
	}
}

static void createCameraAndItsController() {
	UVCameraUtility::CAMERA_DESC desc;
	desc.farPlane = 100.0f;
	desc.nearPlane = 0.1f;
	desc.fov = 0.6f;
	desc.position = { 0.0f, 0.0f, -5.0f };
	desc.lookAt = { 0.0f, 0.0f, 0.0f };
	desc.height = gHeight;
	desc.width = gWidth;
	auto [c, ctrl] = UVCameraUtility::CreateCamera(desc, UVCameraUtility::CONTROLLER_TYPE_EPIC);
	GlobalData.camera.reset(c);
	GlobalData.cameraController.reset(ctrl);
}

static void Setup() {
	MImage::Image::Init();
	GlobalData.pWindow = setupIMGUI(gWidth, gHeight, "test");
	createCameraAndItsController();
	GlobalData.pBackend = RenderBackend::Get();
	GlobalData.pBackend->Initialize();
	DX12BackendUsedData bkData = { reinterpret_cast<size_t>(glfwGetWin32Window(GlobalData.pWindow)), gWidth, gHeight };
	GlobalData.pDevice = GlobalData.pBackend->CreateDevice(&bkData);
	GlobalData.pDevice->Initialize("");
	setupIMGUI_Callback(GlobalData.pWindow, GlobalData.pDevice, GlobalData.pBackend, mousebutton_callback, key_callback);
	glfwSetCursorPosCallback(GlobalData.pWindow, cursor_position_callback);
}

static void Shutdown() {
	shutdownIMGUI(GlobalData.pWindow, GlobalData.pDevice);
	MImage::Image::Shutdown();
}

float VTXBufferData[] = {
	-1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 1.0f
};

uint32_t IDXBufferData[] = {
	0, 1, 2,
	0, 2, 3
};

RasterizeOptions CCWRasterizeOptions() {
	RasterizeOptions ret = RasterizeOptions();
	ret.counterClockWiseIsFront = true;
	return ret;
}

int main() {

	Setup();

	std::filesystem::path modelPath = GET_FILE_PATH_REATIVE_TO_THIS_FILE("DragonAttenuation.glb");
	std::unique_ptr<IMeshResource> mesh = MeshLoaderGLTF::GetInstance().LoadMeshFromFile(modelPath, 1);

	std::unique_ptr<MImage::Image> img = MImage::Image::LoadImageFromFile(GET_FILE_PATH_REATIVE_TO_THIS_FILE("UV_Grid_Sm.jpg"));
	img->ConvertPixelFormat(MImage::IMAGE_FORMAT_R8G8B8A8);

	CommandUnit* cmdUnit = GlobalData.pDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);

	std::unique_ptr<Texture2D> texture(GlobalData.pDevice->CreateTexture2D(img->Width(), img->Height(), 1, 1, UnknownVision::ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM,
		ResourceStatus(RESOURCE_USAGE_SHADER_RESOURCE, RESOURCE_FLAG_STABLY)));

	UnknownVision::ImageDesc desc;
	desc.data = img->GetData(0);
	desc.width = img->Width();
	desc.height = img->Height();
	desc.depth = 1;
	desc.rowPitch = img->GetRowPitch(0);
	desc.slicePitch = img->GetSlicePitch(0);
	GlobalData.pDevice->WriteToTexture2D({ desc }, texture.get(), cmdUnit);

	cmdUnit->TransferState(texture.get(), RESOURCE_STATE_SHADER_RESOURCE);
	cmdUnit->Flush(true);

	MeshPass mp;
	VertexBuffersHolder vtxBuffers = MeshPass::VertexShaderType::VertexFactoryType::CreateVertexBuffersHolder(mesh.get(), cmdUnit, GlobalData.pDevice);
	mp.Init(cmdUnit, GlobalData.pDevice, GlobalData.pBackend);

	Viewport vp;
	{
		ViewportDesc vpDesc;
		vpDesc.topLeftX = 0; vpDesc.topLeftY = 0;
		vpDesc.width = gWidth; vpDesc.height = gHeight;
		vpDesc.maxDepth = 1.0f; vpDesc.minDepth = 0.0f;

		ScissorRectDesc srDesc;
		srDesc.left = 0; srDesc.right = 0; srDesc.bottom = gHeight; srDesc.right = gWidth;

		vp.Init(vpDesc, srDesc, GlobalData.pDevice);
	}
	

	img.reset();
	while (!glfwWindowShouldClose(GlobalData.pWindow))
	{
		static std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		static std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		end = std::chrono::steady_clock::now();
		GlobalData.deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f;
		GPUResource* rts[] = { GlobalData.pDevice->BackBuffer() };
		cmdUnit->TransferState(GlobalData.pDevice->BackBuffer(), RESOURCE_STATE_RENDER_TARGET);

		ImGui_ImplUVGlfw_NewFrame();
		ImGui_ImplUV_NewFrame();
		ImGui::NewFrame();
		IMGUI_FRAME_FUNC();
		ImGui::Render();

		{
			Viewport::ShaderConstantBuffer vpShaderData;
			{
				auto&& camData = GlobalData.camera->GetCameraData();
				vpShaderData.viewMat = camData.viewMat;
				vpShaderData.position = camData.position;
				vpShaderData.projMat = camData.projMat;
			}
			vp.Update(vpShaderData, cmdUnit, GlobalData.pDevice);
		}
		
		mp.Update(&vtxBuffers, texture.get(), vp.GetGPUBuffer());

		cmdUnit->ClearRenderTarget(GlobalData.pDevice->BackBuffer(), BLUE);
		cmdUnit->ClearDepthStencilBuffer(GlobalData.pDevice->DepthStencilBuffer(), 1.0f, 0);

		mp.Draw(rts, 1, GlobalData.pDevice->DepthStencilBuffer(), cmdUnit, vp.GetViewportDesc(), vp.GetScissorRectDesc());

		ImGui_ImplUV_RenderDrawData(ImGui::GetDrawData(), cmdUnit);
		cmdUnit->TransferState(GlobalData.pDevice->BackBuffer(), RESOURCE_STATE_PRESENT);
		size_t fenceValue = cmdUnit->Flush(false);
		ImGui_ImplUV_FrameEnd(fenceValue);
		GlobalData.pDevice->Present();
		GlobalData.pDevice->UpdatePerFrame();
		GlobalData.pDevice->FreeCommandUnit(&cmdUnit);
		cmdUnit = GlobalData.pDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);
		GlobalData.camera->UpdatePerFrameEnd();
		GlobalData.cameraController->CalledPerFrame(GlobalData.deltaTime);
		glfwPollEvents();
		start = end;
	}

	Shutdown();

	return 0;
}