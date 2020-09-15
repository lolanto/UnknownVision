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


namespace ConvoluteCubeMap {
	class VS : public VertexShader {
	public:
		static std::vector<UnknownVision::VertexAttribute> GetVertexAttributes() {
			return {
				UnknownVision::VertexAttribute(UnknownVision::VERTEX_ATTRIBUTE_TYPE_POSITION, UnknownVision::ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
				0, 0, UnknownVision::VertexAttribute::APPEND_FROM_PREVIOUS),
			};
		}
		VS() : VertexShader(FileNameConcatenation(__FILE__, "ConvoluteCubeMapVS.hlsl")) {}
		virtual ~VS() = default;
		virtual const char* Name() const { return "ConvoluteCubeMapVS.hlsl"; }
		virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
			return {};
		}
	};
	class PS : public PixelShader {
	public:
		PS() : PixelShader(FileNameConcatenation(__FILE__, "ConvoluteCubeMapPS.hlsl")) {}
		virtual ~PS() = default;
		virtual const char* Name() const { return "ConvoluteCubeMapPS.hlsl"; }
		virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
			return {
				{ ShaderParameterSlotDesc::OnlyReadTexture(0, 1),
				ShaderParameterSlotDesc::OnlyReadBuffer(0, 1, 0)},
				{ ShaderParameterSlotDesc::LinearSampler(0, 1) }
			};
		}
	};
}

int main() {
	/** 务必放在首行! */
	RAII_HELPER raii__;
	/** 配置管线 */
	ConvoluteCubeMap::VS vs;
	ConvoluteCubeMap::PS ps;
	GlobalData.pBackend->InitializeShaderObject(&vs);
	GlobalData.pBackend->InitializeShaderObject(&ps);
	auto outputStageFunc = []()->OutputStageOptions {
		OutputStageOptions opts;
		opts.rtvFormats[0] = ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM;
		return opts;
	};
	auto psoPtr(GlobalData.pDevice->BuildGraphicsPipelineObject(&vs, &ps, GDefaultRasterizeOptions, outputStageFunc, ConvoluteCubeMap::VS::GetVertexAttributes));
	if (psoPtr == nullptr) abort();
	auto cmdUnit = GlobalData.pDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);
	std::unique_ptr<BindingBoard> bindingBoardForPS(GlobalData.pDevice->RequestBindingBoard(2, DEFAULT_COMMAND_UNIT));
	if (bindingBoardForPS == nullptr) abort();
	// 尝试读取一张照片，然后将它存储到文件
	std::unique_ptr<MImage::Image> img = MImage::Image::LoadImageFromFile(FileNameConcatenation(__FILE__, "env.dds"));
	if (img == nullptr) abort();
	std::unique_ptr<Texture2D> inputTexPtr(GlobalData.pDevice->CreateTexture2D(img->Width(), img->Height(), 1, 6, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM,
		ResourceStatus(RESOURCE_USAGE_SHADER_RESOURCE, RESOURCE_FLAG_STABLY)));
	if (inputTexPtr == nullptr) abort();
	{
		std::vector<std::vector<ImageDesc>> desces(1, std::vector<ImageDesc>(6));
		for (size_t i = 0; i < 6; ++i) {
			desces[0][i].data = img->GetData(0, i);
			desces[0][i].depth = 1;
			desces[0][i].height = img->Height(0, i);
			desces[0][i].width = img->Width(0, i);
			desces[0][i].rowPitch = img->GetRowPitch(0, i);
			desces[0][i].slicePitch = img->GetSlicePitch(0, i);
		}
		if (GlobalData.pDevice->WriteToTexture2DArr(desces, inputTexPtr.get(), cmdUnit) == false) {
			abort();
		}
	}
	std::unique_ptr<Buffer> IdxBuffer;
	std::unique_ptr<Buffer> vtxBuffer;
	{
		uint32_t idxData[] = { 0, 1, 2, 0, 2, 3 };
		float vtxData[] = { 
			-1.0f, 1.0f, 0.0f,
			1.0f, 1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
			-1.0f, -1.0f, 0.0f
		};
		IdxBuffer.reset(GlobalData.pDevice->CreateBuffer(6, sizeof(uint32_t), ResourceStatus(RESOURCE_USAGE_INDEX_BUFFER, RESOURCE_FLAG_STABLY)));
		if (IdxBuffer == nullptr) abort();
		if (GlobalData.pDevice->WriteToBuffer(idxData, IdxBuffer.get(), sizeof(idxData), 0, cmdUnit) == false) abort();
		vtxBuffer.reset(GlobalData.pDevice->CreateBuffer(4, sizeof(float) * 3, ResourceStatus(RESOURCE_USAGE_VERTEX_BUFFER, RESOURCE_FLAG_STABLY)));
		if (vtxBuffer == nullptr) abort();
		if (GlobalData.pDevice->WriteToBuffer(vtxData, vtxBuffer.get(), sizeof(vtxData), 0, cmdUnit) == false) abort();
	}

	std::unique_ptr<Texture2D> RT(GlobalData.pDevice->CreateTexture2D(img->Width(), img->Height(), 1, 1, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM,
		ResourceStatus(RESOURCE_USAGE_RENDER_TARGET, RESOURCE_FLAG_STABLY)));
	std::unique_ptr<Buffer> controlData(GlobalData.pDevice->CreateBuffer(1, sizeof(uint32_t), ResourceStatus(RESOURCE_USAGE_CONSTANT_BUFFER, RESOURCE_FLAG_FREQUENTLY)));
	if (controlData == nullptr) abort();

	bindingBoardForPS->BindingResource(0, inputTexPtr.get(), SHADER_PARAMETER_TYPE_TEXTURE_R, SHADER_PARAMETER_FLAG_CUBE);
	bindingBoardForPS->BindingResource(1, controlData.get(), SHADER_PARAMETER_TYPE_BUFFER_R);
	bindingBoardForPS->Close();

	ViewPort vp;
	vp.maxDepth = 1.0f; vp.minDepth = 0.0f;
	vp.width = img->Width(); vp.height = img->Height();
	vp.topLeftX = 0; vp.topLeftY = 0;
	ScissorRect sr;
	sr.bottom = img->Height(); sr.top = 0;
	sr.right = img->Width(); sr.left = 0;

	/** 向RT绘制内容 */
	cmdUnit->TransferState(IdxBuffer.get(), RESOURCE_STATE_INDEX_BUFFER);
	cmdUnit->TransferState(vtxBuffer.get(), RESOURCE_STATE_VERTEX_BUFFER);
	cmdUnit->Flush(true);
	GPUResource* rts[] = { RT.get() };
	Buffer* vtxs[] = { vtxBuffer.get() };
	std::string fileName[6] = {
		"posx.dds",
		"negx.dds",
		"posy.dds",
		"negy.dds",
		"posz.dds",
		"negz.dds",
	};
	for (uint32_t axisID = 0; axisID < 6; ++axisID) {
		cmdUnit->TransferState(RT.get(), RESOURCE_STATE_RENDER_TARGET);
		cmdUnit->TransferState(controlData.get(), RESOURCE_STATE_COPY_DEST);
		GlobalData.pDevice->WriteToBuffer(&axisID, controlData.get(), sizeof(axisID), 0, cmdUnit);
		cmdUnit->TransferState(controlData.get(), RESOURCE_STATE_CONSTANT_BUFFER);
		cmdUnit->TransferState(inputTexPtr.get(), RESOURCE_STATE_SHADER_RESOURCE);
		cmdUnit->BindPipeline(psoPtr);
		cmdUnit->BindRenderTargets(rts, 1, nullptr);
		cmdUnit->BindScissorRects(1, &sr);
		cmdUnit->BindViewports(1, &vp);
		cmdUnit->BindIndexBuffer(IdxBuffer.get());
		cmdUnit->BindVertexBuffers(0, 1, vtxs);
		cmdUnit->SetBindingBoard(0, bindingBoardForPS.get());
		cmdUnit->Draw(0, IdxBuffer->Capacity(), 0);
		cmdUnit->TransferState(RT.get(), RESOURCE_STATE_COPY_SRC);
		cmdUnit->Flush(true);

		/** 输出RT数据 */
		std::vector<uint8_t> texData;
		if (GlobalData.pDevice->ReadFromTexture2D(texData, RT.get(), cmdUnit) == false) {
			abort();
		}
		LOG_INFO("Tex data size is %d bytes", texData.size());
		std::unique_ptr<MImage::Image> imagePtr = MImage::Image::LoadImageFromMemory(texData.data(), RT->Width(), RT->Height(), MImage::IMAGE_FORMAT_R8G8B8A8);
		if (imagePtr == nullptr) {
			abort();
		}
		if (MImage::Image::StoreImageToFile(imagePtr.get(), FileNameConcatenation(__FILE__, fileName[axisID].c_str())) == false) {
			abort();
		}
	}
	system("pause");
	return 0;
}