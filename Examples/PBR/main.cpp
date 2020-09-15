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
#include "Component.h"
#include "PipelineEntity.h"
#include "../Utility/Image/Image.h"
#include "../Utility/SimpleLight/SimpleLight.h"

#include <../Utility/InfoLog/InfoLog.h>
#include <iostream>
#include <random>
#include <stack>
#include <iostream>
using namespace UnknownVision;

constexpr uint32_t gWidth = 1280;
constexpr uint32_t gHeight = 800;

const float BLUE[4] = { 0.2f, 0.4f, 0.8f, 1.0f };

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

struct RAII_HELPER {
	RAII_HELPER() { Setup(); ResourceCenter::GetCenter().Init(GlobalData.pDevice); }
	~RAII_HELPER() { ResourceCenter::GetCenter().Release(); Shutdown(); }
};

#include "../Shaders/PBRWithSimpleLightShader.hpp"

int main() {
	auto filePathHelper = [](const char* root, const char* file = nullptr)->std::filesystem::path {
		std::filesystem::path p(root);
		if (file) p.replace_filename(file);
		return p;
	};
	RAII_HELPER raii__;

	ResourceCenter::GetCenter().LoadMeshes(filePathHelper(__FILE__, "sphere.obj"));

	std::vector<PipelineEntity> TargetObject(ResourceCenter::GetCenter().NumberOfSubresources(filePathHelper(__FILE__, "sphere.obj")));
	for (size_t i = 0; i < TargetObject.size(); ++i) {
		TargetObject[i].SetComponent(MeshComponent::Create(filePathHelper(__FILE__, "sphere.obj"), i));
		TargetObject[i].SetComponent(PBRTextureSet::Create(
			filePathHelper(__FILE__, "Normal.png"),
			filePathHelper(__FILE__, "hw_BaseColor.png"),
			filePathHelper(__FILE__, "hw_ARM.png")));
		TargetObject[i].SetComponent(ITransformComponent::Create({ 0.0f, 0.0f, 0.0f }));
	}

	std::unique_ptr<SimpleLight::IPointLight> pointLight = SimpleLight::IPointLight::Create({ 0.0f, 0.0f, 5.0f }, { 1.0f, 1.0f, 1.0f }, 1.5f);
	std::unique_ptr<SimpleLight::IPointLight> pointLights[4] = {
		SimpleLight::IPointLight::Create({ -10.0f, 10.0f, -10.0f }, { 1.0f, 1.0f, 1.0f }, 2.0f),
		SimpleLight::IPointLight::Create({ 10.0f, 10.0f, -10.0f }, { 1.0f, 1.0f, 1.0f }, 2.0f),
		SimpleLight::IPointLight::Create({ -10.0f, -10.0f, -10.0f }, { 1.0f, 1.0f, 1.0f }, 2.0f),
		SimpleLight::IPointLight::Create({ 10.0f, -10.0f, -10.0f }, { 1.0f, 1.0f, 1.0f }, 2.0f)
	};
	std::unique_ptr<SimpleLight::IDirectionLight> directionLight = SimpleLight::IDirectionLight::Create({ 0.0f, 0.0f, -5.0f }, { 0.7f, 0.0f, -0.7f }, { 2.0f, 2.0f, 2.0f }, 1.0f);

	PBRWithSimpleLight::VS vs;
	PBRWithSimpleLight::PS ps;
	GlobalData.pBackend->InitializeShaderObject(&vs);
	GlobalData.pBackend->InitializeShaderObject(&ps);
	auto pso = GlobalData.pDevice->BuildGraphicsPipelineObject(&vs, &ps, GDefaultRasterizeOptions, GOutputStageOptionsWithDepthTest1_1, PBRWithSimpleLight::VS::GetVertexAttributes);
	
	PBRWithSimpleLight::ControlPanel cpData;
	cpData.Albedo = { 1.0f, 1.0f, 1.0f, 1.0f }; cpData.Roughness = 1.0f; cpData.Metallic = 0.0f; cpData.NormalPower = 0.1f;
	CommandUnit* cmdUnit = GlobalData.pDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);
	// env-maps
	std::unique_ptr<Texture2D> envMapPtr(GlobalData.pDevice->CreateTexture2D(2048, 2048, 1, 6, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM,
		ResourceStatus(RESOURCE_USAGE_SHADER_RESOURCE, RESOURCE_FLAG_STABLY)));
	{
		std::unique_ptr<MImage::Image> envImgs = MImage::Image::LoadImageFromFile(FileNameConcatenation(__FILE__, "envMap_C.dds"));
		std::vector<std::vector<ImageDesc>> imgDesces(6, std::vector<ImageDesc>(1));
		for (int i = 0; i < 6; ++i) {
			imgDesces[i][0].data = envImgs->GetData(0, i);
			imgDesces[i][0].depth = 1;
			imgDesces[i][0].height = 2048;
			imgDesces[i][0].width = 2048;
			imgDesces[i][0].rowPitch = envImgs->GetRowPitch(0, i);
			imgDesces[i][0].slicePitch = envImgs->GetSlicePitch(0, i);
		}
		if (GlobalData.pDevice->WriteToTexture2DArr(imgDesces, envMapPtr.get(), cmdUnit) == false) abort();
	}

	std::unique_ptr<BindingBoard> bindingBoardForVS(GlobalData.pDevice->RequestBindingBoard(2, DEFAULT_COMMAND_UNIT));
	std::unique_ptr<BindingBoard> bindingBoardForPS(GlobalData.pDevice->RequestBindingBoard(9, DEFAULT_COMMAND_UNIT));
	std::unique_ptr<Buffer> cameraDataBuffer(GlobalData.pDevice->CreateBuffer(1, sizeof(UVCameraUtility::GeneralCameraDataStructure), ResourceStatus(RESOURCE_USAGE_CONSTANT_BUFFER, RESOURCE_FLAG_FREQUENTLY)));
	std::unique_ptr<Buffer> lightDataBuffer(GlobalData.pDevice->CreateBuffer(4, sizeof(SimpleLight::LightDataBufferStructure), ResourceStatus(RESOURCE_USAGE_CONSTANT_BUFFER, RESOURCE_FLAG_FREQUENTLY)));
	std::unique_ptr<Buffer> controlPanelBuffer(GlobalData.pDevice->CreateBuffer(1, sizeof(PBRWithSimpleLight::ControlPanel), ResourceStatus(RESOURCE_USAGE_CONSTANT_BUFFER, RESOURCE_FLAG_FREQUENTLY)));
	{
		auto lightData = pointLight->LightPropertyBuffer();
		std::vector<uint8_t> ld(4 * sizeof(SimpleLight::LightDataBufferStructure));
		for (int i = 0; i < 4; ++i) {
			memcpy(ld.data() + sizeof(SimpleLight::LightDataBufferStructure) * i, pointLights[i]->LightPropertyBuffer().data(), sizeof(SimpleLight::LightDataBufferStructure));
		}
		GlobalData.pDevice->WriteToBuffer(ld.data(), lightDataBuffer.get(), ld.size(), 0, cmdUnit);
	}
	for (size_t i = 0; i < NUMBER_OF_BACK_BUFFERS; ++i)
		cmdUnit->TransferState(GlobalData.pDevice->DepthStencilBuffer(i), RESOURCE_STATE_DEPTH_WRITE);
	cmdUnit->Flush(true);


	bindingBoardForPS->BindingResource(0, lightDataBuffer.get(), SHADER_PARAMETER_TYPE_BUFFER_R);
	bindingBoardForPS->BindingResource(1, cameraDataBuffer.get(), SHADER_PARAMETER_TYPE_BUFFER_R);
	bindingBoardForPS->BindingResource(2, controlPanelBuffer.get(), SHADER_PARAMETER_TYPE_BUFFER_R);
	{
		auto ptrPBRTextureSet = TargetObject[0].GetComponent<PBRTextureSet>();
		bindingBoardForPS->BindingResource(3, ptrPBRTextureSet->BaseColorTexture(), SHADER_PARAMETER_TYPE_TEXTURE_R);
		bindingBoardForPS->BindingResource(4, ptrPBRTextureSet->NormalTexture(), SHADER_PARAMETER_TYPE_TEXTURE_R);
		bindingBoardForPS->BindingResource(5, ptrPBRTextureSet->AO_Roughness_Metallic(), SHADER_PARAMETER_TYPE_TEXTURE_R);
		bindingBoardForPS->BindingResource(6, envMapPtr.get(), SHADER_PARAMETER_TYPE_TEXTURE_R, SHADER_PARAMETER_FLAG_CUBE, 0);
		bindingBoardForPS->BindingResource(7, envMapPtr.get(), SHADER_PARAMETER_TYPE_TEXTURE_R, SHADER_PARAMETER_FLAG_CUBE, 0);
	}
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

		ImGui::Begin("ControlPanel");
		ImGui::ColorEdit3("Albedo", cpData.Albedo.data);
		ImGui::DragFloat("Roughness", &cpData.Roughness, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Metallic", &cpData.Metallic, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("NormalPower", &cpData.NormalPower, 0.01f, 0.001f, 1.0f);
		ImGui::End();

		ImGui::Render();
		cmdUnit->TransferState(controlPanelBuffer.get(), RESOURCE_STATE_COPY_DEST);
		GlobalData.pDevice->WriteToBuffer(&cpData, controlPanelBuffer.get(), sizeof(cpData), 0, cmdUnit);
		cmdUnit->TransferState(controlPanelBuffer.get(), RESOURCE_STATE_CONSTANT_BUFFER);
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
		for (auto& object : TargetObject) {
			bindingBoardForVS->Reset();
			bindingBoardForVS->BindingResource(0, object.GetComponent<ITransformComponent>()->ModelMatrix(GlobalData.pDevice), SHADER_PARAMETER_TYPE_BUFFER_R);
			bindingBoardForVS->BindingResource(1, cameraDataBuffer.get(), SHADER_PARAMETER_TYPE_BUFFER_R);
			bindingBoardForVS->Close();
			cmdUnit->SetBindingBoard(0, bindingBoardForVS.get());
			cmdUnit->BindVertexBuffers(0, 1, object.GetComponent<MeshComponent>()->VertexBuffers().data());
			cmdUnit->BindIndexBuffer(object.GetComponent<MeshComponent>()->IndexBuffer());
			cmdUnit->Draw(0, object.GetComponent<MeshComponent>()->IndexBuffer()->Capacity(), 0);
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

	return 0;
}