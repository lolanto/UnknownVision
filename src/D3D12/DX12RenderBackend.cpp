#include "DX12RenderBackend.h"

#include "../Utility/WindowBase/win32/WindowWin32.h"
#include "../Utility/InfoLog/InfoLog.h"

#include <list>

#define XifFailed(function, behavior) if (FAILED(function)) behavior

BEG_NAME_SPACE
bool DX12RenderBackend::Initialize()
{
	if (m_isInitialized) return true;

	HRESULT hr = S_OK;
	/** 创建DXGI工厂!  该过程可用于激活Debug Layer */
	UINT factoryFlag = 0;
#ifdef _DEBUG
	SmartPTR<ID3D12Debug3> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		factoryFlag |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif // DEBUG
	if (FAILED(CreateDXGIFactory2(factoryFlag, IID_PPV_ARGS(&m_factory)))) {
		MLOG("Create DXGI factory FAILED!");
		return false;
	}

	return m_isInitialized = true;
}

RenderDevice * DX12RenderBackend::CreateDevice(void * parameters)
{
	if (!m_isInitialized) return nullptr;
	SmartPTR<ID3D12Device> device;
	SmartPTR<ID3D12CommandQueue> cmdQueue;
	SmartPTR<IDXGISwapChain1> swapChain;
	DX12BackendUsedData datas = *reinterpret_cast<DX12BackendUsedData*>(parameters);
	const WindowWin32* win = std::get<0>(datas);
	uint32_t width = std::get<1>(datas);
	uint32_t height = std::get<2>(datas);
	/** 枚举设备，创建device */
	{
		SmartPTR<IDXGIAdapter4> adapter;
		for (UINT adapterIdx = 0; m_factory->EnumAdapters(adapterIdx,
			reinterpret_cast<IDXGIAdapter**>(adapter.ReleaseAndGetAddressOf())) != DXGI_ERROR_NOT_FOUND; ++adapterIdx) {
			DXGI_ADAPTER_DESC3 adapterDesc;
			adapter->GetDesc3(&adapterDesc);
			if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue; /**< 不使用软件模拟的设备 */
			/** 检查该设备是否支持dx12 */
			XifFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)), {
				return nullptr;
				});
			break;
		}
	}
	/** 创建关键指令提交队列 */
	{
		D3D12_COMMAND_QUEUE_DESC queDesc;
		queDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queDesc.NodeMask = 0; /**< 不使用多设备 */
		queDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH; /**< CommandQueue需要分配优先级，这里测试使用高优先级 */
		XifFailed(device->CreateCommandQueue(&queDesc, IID_PPV_ARGS(&cmdQueue)), {
			return nullptr;
			});
	}
	/** 创建交换链 */
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE; /**< 指定交换链缓冲的透明行为，暂且不进行特殊设置 */
		swapChainDesc.BufferCount = NUMBER_OF_BACK_BUFFERS; /**< 指定后台缓冲的数量 */
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT; /**< 交换链缓冲的用途，默认已有backbuffer */
		swapChainDesc.Flags = 0; /**< 不进行特殊设置 */
		swapChainDesc.Format = BACK_BUFFER_FORMAT;
		swapChainDesc.Height = height;
		swapChainDesc.Width = width;
		/** 不进行多重采样 */
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE; /**< 当窗口被拉伸时不进行适应性操作 */
		swapChainDesc.Stereo = false;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		XifFailed(m_factory->CreateSwapChainForHwnd(cmdQueue.Get(),
			win->hWnd(), &swapChainDesc,
			nullptr,  /**< 不使用全屏，可直接传入null*/
			nullptr, /**< 暂时不对输出做任何处理*/
			swapChain.GetAddressOf()), {
				return nullptr;
			});
	}
	SmartPTR<IDXGISwapChain3> sc3;
	swapChain.As(&sc3);
	m_devices.push_back(new DX12RenderDevice(*this, cmdQueue, sc3, device, width, height));
	return m_devices.back();
}

bool DX12RenderBackend::InitializeShaderObject(BasicShader* shader) {
	/** 为shader分配shader handle */
	shader->m_handle = m_shaderManager.Compile(shader->m_shaderFile, shader->GetShaderType());
	return true;
}

GraphicsPipelineObject* DX12RenderBackend::BuildGraphicsPipelineObject(
	VertexShader* vs, PixelShader* ps,
	RasterizeOptionsFunc rastOpt, OutputStageOptionsFunc outputOpt,
	VertexAttributesFunc vtxAttribList) {
	/** 构造GraphicsPipeline不能缺少VS和PS，必须提前进行初始化 */
	if (vs->m_handle == ShaderHandle::InvalidIndex() ||
		ps->m_handle == ShaderHandle::InvalidIndex()) {
		return nullptr;
	}
	DX12GraphicsPipelineObject graphicsPSO(vs, ps, rastOpt, outputOpt, vtxAttribList);
	auto dx12vs = m_shaderManager[vs->GetHandle()];
	auto dx12ps = m_shaderManager[ps->GetHandle()];
	if (m_pipelineManager.Build(graphicsPSO, dx12vs, dx12ps) == false) return nullptr;	
}

//auto DX12RenderBackend::UpdateShaderInfo(const char * shaderName, ShaderType typeHint)
//	-> const DX12RenderBackend::ShaderInfo&
//{
//	decltype(m_shaders)::iterator iterator;
//	/** TODO: 这种加锁方式在只有一个shader需要创建时，
//	 * 其余无关shader的查询相关的调用都会被阻塞，应该优化 */
//	std::lock_guard<std::mutex> lg(m_shaderLock);
//	/** 判断shader是否已经被缓存 */
//	iterator = m_shaders.find(shaderName);
//	if (iterator != m_shaders.end()) {
//		FLOG("%s has been exist!\n", shaderName);
//	}
//	else {
//		auto[tempitr, res] = m_shaders.insert(std::make_pair(shaderName, ShaderInfo()));
//		assert(res == true);
//		iterator = tempitr;
//	}
//	ShaderInfo& shader = iterator->second;
//	DXCompilerHelper dxc;
//	auto[timestamp, isExist] = dxc.TimeStampOfShaderSourceCode(shaderName);
//	assert(isExist);
//	if (shader.timestamp <= timestamp) {
//		/** 缓存的字节码失效，需要重新加载 */
//		/** 清除原有的shader缓存数据 */
//		shader.Reset();
//		/** 加载新的blob */
//		/** 根据shader类型构造profile */
//		switch (typeHint) {
//		case SHADER_TYPE_VERTEX_SHADER:
//			shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(vs));
//			break;
//		case SHADER_TYPE_PIXEL_SHADER:
//			shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(ps));
//			break;
//		case SHADER_TYPE_COMPUTE_SHADER:
//			shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(cs));
//			break;
//		case SHADER_TYPE_GEOMETRY_SHADER:
//			shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(gs));
//			break;
//		case SHADER_TYPE_TESSELLATION_SHADER:
//			shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(ds));
//			break;
//		case SHADER_TYPE_HULL_SHADER:
//			shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(hs));
//			break;
//		default:
//			assert(false);
//		}
//		if (shader.blob == nullptr) {
//			MLOG(dxc.LastErrorMsg());
//			assert(false);
//		}
//		shader.timestamp = timestamp + 1; /**< 更新时间戳 */
//		/** 对字节码重新进行分析 */
//		SmartPTR<ID3D12ShaderReflection> ref = dxc.RetrieveShaderDescriptionFromByteCode(shader.blob);
//		D3D12_SHADER_DESC shaderDesc;
//		ref->GetDesc(&shaderDesc);
//		shader.version = shaderDesc.Version;
//		/** TODO: 支持更多类型shader的semantic */
//		if (typeHint == SHADER_TYPE_VERTEX_SHADER) {
//			for (unsigned int i = 0; i < shaderDesc.InputParameters; ++i) {
//				D3D12_SIGNATURE_PARAMETER_DESC sigParaDesc;
//				ref->GetInputParameterDesc(i, &sigParaDesc);
//				if (strcmp(sigParaDesc.SemanticName, "POSITION") == 0) {
//					if (sigParaDesc.SemanticIndex >= getPOSITIONN(shader.VS_IO))
//						setPOSITIONN(shader.VS_IO, sigParaDesc.SemanticIndex + 1);
//				}
//				else if (strcmp(sigParaDesc.SemanticName, "TEXCOORD") == 0) {
//					if (sigParaDesc.SemanticIndex >= getTEXCOORDN(shader.VS_IO))
//						setTEXCOORDN(shader.VS_IO, sigParaDesc.SemanticIndex + 1);
//				}
//				else if (strcmp(sigParaDesc.SemanticName, "NORMAL") == 0) {
//					if (sigParaDesc.SemanticIndex >= getNORMALN(shader.VS_IO))
//						setNORMALN(shader.VS_IO, sigParaDesc.SemanticIndex + 1);
//				}
//				else if (strcmp(sigParaDesc.SemanticName, "TANGENT") == 0) {
//					if (sigParaDesc.SemanticIndex >= getTANGENTN(shader.VS_IO))
//						setTANGENTN(shader.VS_IO, sigParaDesc.SemanticIndex + 1);
//				}
//				else if (strcmp(sigParaDesc.SemanticName, "COLOR") == 0) {
//					if (sigParaDesc.SemanticIndex >= getCOLORN(shader.VS_IO))
//						setCOLORN(shader.VS_IO, sigParaDesc.SemanticIndex + 1);
//				}
//				else {
//					FLOG("%s: Doesn't support %s currently\n", __FUNCTION__, sigParaDesc.SemanticName);
//				}
//			}
//		}
//		else if (typeHint == SHADER_TYPE_PIXEL_SHADER) {
//			for (unsigned int i = 0; i < shaderDesc.OutputParameters; ++i) {
//				D3D12_SIGNATURE_PARAMETER_DESC sigParaDesc;
//				ref->GetOutputParameterDesc(i, &sigParaDesc);
//				if (strcmp(sigParaDesc.SemanticName, "SV_TARGET") == 0) {
//					if (sigParaDesc.SemanticIndex >= getSV_TARGET(shader.PS_IO))
//						setSV_TARGET(shader.PS_IO, sigParaDesc.SemanticIndex + 1);
//				}
//				else if (strcmp(sigParaDesc.SemanticName, "SV_DEPTH") == 0) {
//					if (sigParaDesc.SemanticIndex >= getSV_DEPTH(shader.PS_IO))
//						setSV_DEPTH(shader.PS_IO, sigParaDesc.SemanticIndex + 1);
//				}
//			}
//		}
//		/** 分析constant buffer, texture以及sampler */
//		for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
//			D3D12_SHADER_INPUT_BIND_DESC resDesc;
//			ref->GetResourceBindingDesc(i, &resDesc);
//
//			shader.signatures.insert(std::make_pair(resDesc.Name,
//				ShaderInfo::ResourceInfo(resDesc.Type, resDesc.BindPoint, resDesc.Space)));
//		}
//	}
//	return shader;
//}


END_NAME_SPACE
