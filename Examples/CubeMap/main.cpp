#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <GraphicsInterface/RenderBackend.h>
#include <GraphicsInterface/RenderDevice.h>
#include <GraphicsInterface/Pipeline.h>
#include <GraphicsInterface/BindingBoard.h>
#include "../Utility/IMGUI_IMPL/imgui_impl_uv_helper.h"
#include "../Utility/GeneralCamera/GeneralCamera.h"
#include <../Utility/MathInterface/MathInterface.hpp>
#include "../Utility/Image/Image.h"
#include "../Utility/MeshLoader/MeshLoader.h"
#include <../Utility/InfoLog/InfoLog.h>
#include <iostream>
#include <random>
#include <stack>
#include <iostream>
using namespace UnknownVision;

constexpr uint32_t gWidth = 1280;
constexpr uint32_t gHeight = 800;

const float BLUE[4] = { 0.2f, 0.4f, 0.8f, 1.0f };

/** 全局数据，用来在回调函数内传递数据 */
struct {
	std::unique_ptr<UVCameraUtility::ICamera> camera;
	std::unique_ptr<UVCameraUtility::ICameraController> cameraController;
	IMath::IDOUBLE2 mouse_pos;
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

/** 键盘事件回调函数 */
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_W && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_W, action == GLFW_PRESS);
	if (key == GLFW_KEY_S && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_S, action == GLFW_PRESS);
	if (key == GLFW_KEY_A && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_A, action == GLFW_PRESS);
	if (key == GLFW_KEY_D && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_D, action == GLFW_PRESS);
}
/** 鼠标事件回调函数 */
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
/** 鼠标位置变更回调函数 */
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (GlobalData.camera != nullptr && GlobalData.cameraController != nullptr) {
		GlobalData.mouse_pos = { xpos, ypos };
		GlobalData.cameraController->MouseCallback((float)xpos, (float)ypos, UVCameraUtility::MOUSE_BUTTON_NONE, false);
	}
}
/** 摄像机创建相关调用 */
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
/** 所有需要提前初始化的全局对象指令都存于此 */
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
/** 所有需要在程序结束时调用的函数都存于此 */
static void Shutdown() {
	shutdownIMGUI(GlobalData.pWindow, GlobalData.pDevice);
	MImage::Image::Shutdown();
}
/** 负责确保程序执行前和程序结束时调用相关函数的结构体 */
struct RAII_HELPER {
	RAII_HELPER() { Setup(); }
	~RAII_HELPER() { Shutdown(); }
};

#include "../Shaders/CubeMapTestShader.hpp"

int main() {
	/** 务必放在首行! */
	RAII_HELPER raii__;

	/** 配置管线，包括初始化shader和PSO */
	CubeMapTestShader::VS vs;
	CubeMapTestShader::PS ps;
	GlobalData.pBackend->InitializeShaderObject(&vs);
	GlobalData.pBackend->InitializeShaderObject(&ps);
	auto pso = GlobalData.pDevice->BuildGraphicsPipelineObject(&vs, &ps, GDefaultRasterizeOptions, GOutputStageOptionsWithDepthTest1_1, CubeMapTestShader::VS::GetVertexAttributes);

	CommandUnit* cmdUnit = GlobalData.pDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);

	/** 准备管线数据 */
	
	auto meshLoader_ptr = MeshLoader::IMeshLoader::GetLoader(MeshLoader::MeshLoaderConfigurationDesc::PN());
	std::vector<MeshLoader::MeshDataContainer> output;
	if (meshLoader_ptr->Load(FileNameConcatenation(__FILE__, "sphere.obj"), output) == false) { abort(); }

	std::unique_ptr<Buffer> vtxBuffer(GlobalData.pDevice->CreateBuffer(output[0].numVertices, output[0].elementStrides[0],
		ResourceStatus(RESOURCE_USAGE_VERTEX_BUFFER, RESOURCE_FLAG_STABLY)));
	GlobalData.pDevice->WriteToBuffer(output[0].vtxBuffers[0].data(), vtxBuffer.get(), output[0].vtxBuffers[0].size(), 0, cmdUnit);
	std::unique_ptr<Buffer> idxBuffer(GlobalData.pDevice->CreateBuffer(output[0].numIndices, MeshLoader::INDEX_DATA_SIZE,
		ResourceStatus(RESOURCE_USAGE_INDEX_BUFFER, RESOURCE_FLAG_STABLY)));
	GlobalData.pDevice->WriteToBuffer(output[0].idxBuffer.data(), idxBuffer.get(), output[0].idxBuffer.size() * MeshLoader::INDEX_DATA_SIZE, 0, cmdUnit);

	cmdUnit->TransferState(vtxBuffer.get(), RESOURCE_STATE_VERTEX_BUFFER);
	cmdUnit->TransferState(idxBuffer.get(), RESOURCE_STATE_INDEX_BUFFER);
	cmdUnit->Flush(true);

	std::unique_ptr<Buffer> cameraDataBuffer(GlobalData.pDevice->CreateBuffer(1, sizeof(UVCameraUtility::GeneralCameraDataStructure), ResourceStatus(RESOURCE_USAGE_CONSTANT_BUFFER, RESOURCE_FLAG_FREQUENTLY)));
	std::unique_ptr<MImage::Image> envImg = MImage::Image::LoadImageFromFile(FileNameConcatenation(__FILE__, "env.dds"));
	std::unique_ptr<Texture2D> cubemapPtr(GlobalData.pDevice->CreateTexture2D(envImg->Width(), envImg->Height(), 1, 6, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM,
		ResourceStatus(RESOURCE_USAGE_SHADER_RESOURCE, RESOURCE_FLAG_STABLY)));
	{
		std::vector<std::vector<ImageDesc>> imgDescs(6, std::vector<ImageDesc>(1));
		for (size_t i = 0; i < 6; ++i) {
			imgDescs[i][0].data = envImg->GetData(0, i);
			imgDescs[i][0].depth = 1;
			imgDescs[i][0].height = envImg->Height(0, i);
			imgDescs[i][0].width = envImg->Width(0, i);
			imgDescs[i][0].rowPitch = envImg->GetRowPitch(0, i);
			imgDescs[i][0].slicePitch = envImg->GetSlicePitch(0, i);
		}
		if (GlobalData.pDevice->WriteToTexture2DArr(imgDescs, cubemapPtr.get(), cmdUnit) == false) {
			abort();
		}
	}

	std::unique_ptr<BindingBoard> bindingBoardForVS(GlobalData.pDevice->RequestBindingBoard(1, DEFAULT_COMMAND_UNIT));
	std::unique_ptr<BindingBoard> bindingBoardForPS(GlobalData.pDevice->RequestBindingBoard(1, DEFAULT_COMMAND_UNIT));
	bindingBoardForVS->BindingResource(0, cameraDataBuffer.get(), SHADER_PARAMETER_TYPE_BUFFER_R);
	bindingBoardForPS->BindingResource(0, cubemapPtr.get(), SHADER_PARAMETER_TYPE_TEXTURE_R, SHADER_PARAMETER_FLAG_CUBE);
	bindingBoardForPS->Close();
	bindingBoardForVS->Close();

	ViewPort vp;
	vp.topLeftX = 0; vp.topLeftY = 0;
	vp.width = gWidth; vp.height = gHeight;
	vp.maxDepth = 1.0f; vp.minDepth = 0.0f;

	ScissorRect sr;
	sr.left = 0; sr.top = 0; sr.bottom = gHeight; sr.right = gWidth;

	UVCameraUtility::GeneralCameraDataStructure cameraDatas;
	while (!glfwWindowShouldClose(GlobalData.pWindow))
	{
		static std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		static std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		end = std::chrono::steady_clock::now();
		GlobalData.deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f;
		GPUResource* rts[] = { GlobalData.pDevice->BackBuffer() };
		ImGui_ImplUVGlfw_NewFrame();
		ImGui_ImplUV_NewFrame();
		ImGui::NewFrame();
		IMGUI_FRAME_FUNC();

		ImGui::Render();
		cameraDatas = GlobalData.camera->GetCameraData();
		GlobalData.pDevice->WriteToBuffer(&cameraDatas, cameraDataBuffer.get(), sizeof(cameraDatas), 0, cmdUnit);
		cmdUnit->TransferState(GlobalData.pDevice->BackBuffer(), RESOURCE_STATE_RENDER_TARGET);
		cmdUnit->TransferState(GlobalData.pDevice->DepthStencilBuffer(), RESOURCE_STATE_DEPTH_WRITE);
		cmdUnit->ClearRenderTarget(GlobalData.pDevice->BackBuffer(), BLUE);
		cmdUnit->ClearDepthStencilBuffer(GlobalData.pDevice->DepthStencilBuffer());
		cmdUnit->BindRenderTargets(rts, 1, GlobalData.pDevice->DepthStencilBuffer());

		cmdUnit->BindPipeline(pso);
		cmdUnit->BindViewports(1, &vp);
		cmdUnit->BindScissorRects(1, &sr);
		cmdUnit->SetBindingBoard(0, bindingBoardForVS.get());
		cmdUnit->SetBindingBoard(1, bindingBoardForPS.get());
		cmdUnit->BindIndexBuffer(idxBuffer.get());
		Buffer* vtxbufs[] = { vtxBuffer.get() };
		cmdUnit->BindVertexBuffers(0, 1, vtxbufs);
		cmdUnit->Draw(0, idxBuffer->Capacity(), 0);


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

	return 0;
}