#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <GraphicsInterface/RenderBackend.h>
#include <GraphicsInterface/RenderDevice.h>
#include <GraphicsInterface/Pipeline.h>
#include "../Utility/Image/Image.h"
#include <GraphicsInterface/BindingBoard.h>
#include "../Utility/IMGUI_IMPL/imgui_impl_uv_helper.h"
#include "../Utility/GeneralCamera/GeneralCamera.h"
#include <../Utility/MathInterface/MathInterface.hpp>
#include <iostream>
#include <random>
#include <stack>
#include <iostream>
using namespace UnknownVision;

constexpr uint32_t gWidth = 1280	;
constexpr uint32_t gHeight = 800;

const float BLUE[4] = { 0.2f, 0.4f, 0.8f, 1.0f };
const float BLACK[4] = { 0.0, 0.0, 0.0, 0.0 };

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

inline OutputStageOptions RGBA8OutputStatgeOptions() {
	OutputStageOptions res;
	res.enableDepthTest = false;
	res.rtvFormats[0] = ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM;
	res.dsvFormat = ELEMENT_FORMAT_TYPE_UNKNOWN;
	return res;
}


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_W && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_W, action == GLFW_PRESS);
	if (key == GLFW_KEY_S && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_S, action == GLFW_PRESS);
	if (key == GLFW_KEY_A && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_A, action == GLFW_PRESS);
	if (key == GLFW_KEY_D && action != GLFW_REPEAT) GlobalData.cameraController->KeyCallback(UVCameraUtility::KEY_BUTTON_D, action == GLFW_PRESS);
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

const uint32_t CAMERA_JITTER_COUNT = 2;
glm::vec2 CameraJitterOffsets[CAMERA_JITTER_COUNT] = {
	{-0.5,  0.0},
	{ 0.0, 0.0}
};

#include "../Shaders/LoadTextureShader.hpp"
#include "../Shaders/PostProcessShader.hpp"
#include "../Shaders/TAAShader.hpp"

int main() {
	auto filePathHelper = [](const char* root, const char* file = nullptr)->std::filesystem::path {
		std::filesystem::path p(root);
		if (file) p.replace_filename(file);
		return p;
	};
	Setup();
	
	std::unique_ptr<MImage::Image> img = MImage::Image::LoadImageFromFile(filePathHelper(__FILE__, "UV_Grid_Sm.jpg"));
	img->ConvertPixelFormat(MImage::IMAGE_FORMAT_R8G8B8A8);
	LoadTextureVS vs;
	LoadTexturePS ps;
	GlobalData.pBackend->InitializeShaderObject(&vs);
	GlobalData.pBackend->InitializeShaderObject(&ps);
	auto pso = GlobalData.pDevice->BuildGraphicsPipelineObject(&vs, &ps, GDefaultRasterizeOptions, RGBA8OutputStatgeOptions, LoadTextureVS::GetVertexAttributes);
	std::unique_ptr<Buffer> vtxBuffer (GlobalData.pDevice->CreateBuffer(4, sizeof(float) * 5, ResourceStatus(RESOURCE_USAGE_VERTEX_BUFFER, RESOURCE_FLAG_STABLY)));
	std::unique_ptr<Buffer> idxBuffer(GlobalData.pDevice->CreateBuffer(6, sizeof(uint32_t), ResourceStatus(RESOURCE_USAGE_INDEX_BUFFER, RESOURCE_FLAG_STABLY)));
	std::unique_ptr<Texture2D> texture(GlobalData.pDevice->CreateTexture2D(img->Width(), img->Height(), 1, 1, UnknownVision::ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM,
		ResourceStatus(RESOURCE_USAGE_SHADER_RESOURCE, RESOURCE_FLAG_STABLY)));
	std::unique_ptr<BindingBoard> bindingBoardForVS(GlobalData.pDevice->RequestBindingBoard(1, DEFAULT_COMMAND_UNIT));
	std::unique_ptr<BindingBoard> bindingBoardForPS(GlobalData.pDevice->RequestBindingBoard(1, DEFAULT_COMMAND_UNIT));
	std::unique_ptr<Buffer> cameraDataBuffer(GlobalData.pDevice->CreateBuffer(1, sizeof(UVCameraUtility::GeneralCameraDataStructure), ResourceStatus(RESOURCE_USAGE_CONSTANT_BUFFER, RESOURCE_FLAG_FREQUENTLY)));
	CommandUnit* cmdUnit = GlobalData.pDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);
	GlobalData.pDevice->WriteToBuffer(VTXBufferData, vtxBuffer.get(), sizeof(VTXBufferData), 0, cmdUnit);
	GlobalData.pDevice->WriteToBuffer(IDXBufferData, idxBuffer.get(), sizeof(IDXBufferData), 0, cmdUnit);
	UnknownVision::ImageDesc desc;
	desc.data = img->GetData(0);
	desc.width = img->Width();
	desc.height = img->Height();
	desc.depth = 1;
	desc.rowPitch = img->GetRowPitch(0);
	desc.slicePitch = img->GetSlicePitch(0);
	GlobalData.pDevice->WriteToTexture2D({ desc }, texture.get(), cmdUnit);

	std::unique_ptr<Texture2D> MyRTS[2];
	{
		int IDX = 0;
		for (auto& rt : MyRTS) {
			rt.reset((GlobalData.pDevice->CreateTexture2D(gWidth, gHeight, 1, 1, UnknownVision::ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM,
				ResourceStatus(RESOURCE_USAGE_SHADER_RESOURCE | RESOURCE_USAGE_RENDER_TARGET, RESOURCE_FLAG_STABLY))));
			std::wstring name = L"MyRTS" + std::to_wstring(IDX);
			rt->SetName(name.data());
			++IDX;
		}
	}

	std::unique_ptr<Texture2D> ppRT (GlobalData.pDevice->CreateTexture2D(gWidth, gHeight, 1, 1, UnknownVision::ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM,
		ResourceStatus(RESOURCE_USAGE_SHADER_RESOURCE | RESOURCE_USAGE_RENDER_TARGET, RESOURCE_FLAG_STABLY)));
	ppRT->SetName(L"ppRT");

	PostProcessVS pp_vs;
	TAAPS taa_ps;
	GlobalData.pBackend->InitializeShaderObject(&pp_vs);
	GlobalData.pBackend->InitializeShaderObject(&taa_ps);
	auto taa_pso = GlobalData.pDevice->BuildGraphicsPipelineObject(&pp_vs, &taa_ps, GDefaultRasterizeOptions, RGBA8OutputStatgeOptions, decltype(pp_vs)::GetVertexAttributes);
	std::unique_ptr<BindingBoard> taa_bindingBoardForPS(GlobalData.pDevice->RequestBindingBoard(3, DEFAULT_COMMAND_UNIT));
	taa_bindingBoardForPS->Close();

	CopyToTexturePS ctt_ps;
	GlobalData.pBackend->InitializeShaderObject(&ctt_ps);
	auto ctt_pso = GlobalData.pDevice->BuildGraphicsPipelineObject(&pp_vs, &ctt_ps, GDefaultRasterizeOptions, GDefaultOutputStageOptions, decltype(pp_vs)::GetVertexAttributes);
	std::unique_ptr<BindingBoard> ctt_bindingBoardForPS(GlobalData.pDevice->RequestBindingBoard(1, DEFAULT_COMMAND_UNIT));
	ctt_bindingBoardForPS->Close();

	cmdUnit->TransferState(vtxBuffer.get(), RESOURCE_STATE_VERTEX_BUFFER);
	cmdUnit->TransferState(idxBuffer.get(), RESOURCE_STATE_INDEX_BUFFER);
	cmdUnit->TransferState(texture.get(), RESOURCE_STATE_SHADER_RESOURCE);
	cmdUnit->Flush(true);

	bindingBoardForVS->BindingResource(0, cameraDataBuffer.get(), SHADER_PARAMETER_TYPE_BUFFER_R);
	bindingBoardForVS->Close();
	bindingBoardForPS->BindingResource(0, texture.get(), SHADER_PARAMETER_TYPE_TEXTURE_R);
	bindingBoardForPS->Close();

	ViewPort vp;
	vp.topLeftX = 0; vp.topLeftY = 0;
	vp.width = gWidth; vp.height = gHeight;
	vp.maxDepth = 1.0f; vp.minDepth = 0.0f;

	ScissorRect sr;
	sr.left = 0; sr.right = 0; sr.bottom = gHeight; sr.right = gWidth;

	img.reset();
	while (!glfwWindowShouldClose(GlobalData.pWindow))
	{
		static std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		static std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		end = std::chrono::steady_clock::now();
		GlobalData.deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f;
		ImGui_ImplUVGlfw_NewFrame();
		ImGui_ImplUV_NewFrame();
		ImGui::NewFrame();
		IMGUI_FRAME_FUNC();
		ImGui::Render();

		/** Render Scene */
		{
			//GPUResource* rts[] = { GlobalData.pDevice->BackBuffer() };
			GPUResource* rts[] = { MyRTS[GlobalData.pDevice->GetCurrentFrameCount() % 2].get() };
			Buffer* vtxbufs[] = { vtxBuffer.get() };
			GlobalData.camera->SetOffset(CameraJitterOffsets[GlobalData.pDevice->GetCurrentFrameCount() % 2]);
			UVCameraUtility::GeneralCameraDataStructure cam = GlobalData.camera->GetCameraData();
			GlobalData.pDevice->WriteToBuffer(&cam, cameraDataBuffer.get(), sizeof(UVCameraUtility::GeneralCameraDataStructure), 0, cmdUnit);
			cmdUnit->TransferState(rts[0], RESOURCE_STATE_RENDER_TARGET);
			cmdUnit->ClearRenderTarget(rts[0], BLACK);
			cmdUnit->BindRenderTargets(rts, 1, nullptr);

			cmdUnit->BindPipeline(pso);
			cmdUnit->BindViewports(1, &vp);
			cmdUnit->BindScissorRects(1, &sr);
			cmdUnit->BindVertexBuffers(0, 1, vtxbufs);
			cmdUnit->BindIndexBuffer(idxBuffer.get());
			cmdUnit->SetBindingBoard(0, bindingBoardForVS.get());
			cmdUnit->SetBindingBoard(1, bindingBoardForPS.get());
			cmdUnit->Draw(0, 6, 0);
		}

		/** PostProcess */
		{
			cmdUnit->TransferState(MyRTS[0].get(), RESOURCE_STATE_SHADER_RESOURCE);
			cmdUnit->TransferState(MyRTS[1].get(), RESOURCE_STATE_SHADER_RESOURCE);
			taa_bindingBoardForPS->Reset();
			taa_bindingBoardForPS->BindingResource(0, MyRTS[0].get(), SHADER_PARAMETER_TYPE_TEXTURE_R);
			taa_bindingBoardForPS->BindingResource(1, MyRTS[1].get(), SHADER_PARAMETER_TYPE_TEXTURE_R);
			taa_bindingBoardForPS->Close();
			GPUResource* rts[] = { ppRT.get() };
			Buffer* vtxbufs[] = { vtxBuffer.get() };
			cmdUnit->TransferState(rts[0], RESOURCE_STATE_RENDER_TARGET);
			cmdUnit->ClearRenderTarget(rts[0], BLACK);
			cmdUnit->BindRenderTargets(rts, 1, nullptr);

			cmdUnit->BindPipeline(taa_pso);
			cmdUnit->BindViewports(1, &vp);
			cmdUnit->BindScissorRects(1, &sr);
			cmdUnit->BindVertexBuffers(0, 1, vtxbufs);
			cmdUnit->BindIndexBuffer(idxBuffer.get());
			cmdUnit->SetBindingBoard(0, taa_bindingBoardForPS.get());
			cmdUnit->Draw(0, 6, 0);
		}

		/** Copy to Screen */
		{
			cmdUnit->TransferState(ppRT.get(), RESOURCE_STATE_SHADER_RESOURCE);
			ctt_bindingBoardForPS->Reset();
			ctt_bindingBoardForPS->BindingResource(0, ppRT.get(), SHADER_PARAMETER_TYPE_TEXTURE_R);
			ctt_bindingBoardForPS->Close();
			GPUResource* rts[] = { GlobalData.pDevice->BackBuffer() };
			Buffer* vtxbufs[] = { vtxBuffer.get() };
			cmdUnit->TransferState(rts[0], RESOURCE_STATE_RENDER_TARGET);
			cmdUnit->ClearRenderTarget(rts[0], BLUE);
			cmdUnit->BindRenderTargets(rts, 1, nullptr);

			cmdUnit->BindPipeline(ctt_pso);
			cmdUnit->BindViewports(1, &vp);
			cmdUnit->BindScissorRects(1, &sr);
			cmdUnit->BindVertexBuffers(0, 1, vtxbufs);
			cmdUnit->BindIndexBuffer(idxBuffer.get());
			cmdUnit->SetBindingBoard(0, ctt_bindingBoardForPS.get());
			cmdUnit->Draw(0, 6, 0);
		}

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