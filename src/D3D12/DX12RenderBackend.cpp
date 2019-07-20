#include "DX12RenderBasic.h"
#include "../Utility/WindowBase/win32/WindowWin32.h"
#include "../Utility/InfoLog/InfoLog.h"
#include "../Utility/DXCompilerHelper/DXCompilerHelper.h"

#include <d3d12shader.h>
#include <list>

#define XifFailed(function, behavior) if (FAILED(function)) behavior

BEG_NAME_SPACE
bool UnknownVision::DX12RenderBackend::Initialize()
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
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

	m_devices.push_back(new DX12RenderDevice(*this, cmdQueue, swapChain, device, width, height));
	return m_devices.back();
}

ProgramDescriptor DX12RenderBackend::RequestProgram(const ShaderNames& shaderNames, VertexAttributeHandle va_handle,
	bool usedIndex, RasterizeOptions rasterization, OutputStageOptions outputStage) {
	/** 先校验shader组合的合法性 */
	ProgramType newProgramType;
	if (shaderNames[SHADER_TYPE_VERTEX_SHADER].size() != 0 &&
		shaderNames[SHADER_TYPE_PIXEL_SHADER].size() != 0)
		newProgramType = PROGRAM_TYPE_GRAPHICS;
	else if (shaderNames[SHADER_TYPE_COMPUTE_SHADER].size() != 0)
		newProgramType = PROGRAM_TYPE_COMPUTE;
	else {
		FLOG("%s: Invalid shader group!\n", __FUNCTION__);
		return ProgramDescriptor::CreateInvalidDescirptor();
	}

	std::map<std::string, Parameter::Type> signature; /**< 存储该program所有签名 */
	ProgramInfo newProgramInfo;
	/** 一些临时数据，用于先行收集shader的srv, cbv, uav信息，供之后的排序使用 */
	std::list<uint32_t> srv_cbv_uav_idx;
	std::map<std::string, std::list<uint32_t>::iterator> name_srv_cbv_uav_ptr;
	std::list<uint32_t> sampler_idx;
	std::map<std::string, std::list<uint32_t>::iterator> name_sampler_ptr;
	auto listInsert = [](std::list<uint32_t>& list, uint32_t value)->std::list<uint32_t>::iterator {
		auto iter = list.begin();
		while (iter != list.end() && value > (*iter)) ++iter;
		if (iter == list.end()) {
			list.push_back(value);
			return std::prev(list.end());
		}
		else {
			return list.insert(iter, value);
		}
	};
	/** 逐个shader分析 */
	for (uint8_t i = 0; i < SHADER_TYPE_NUMBER_OF_TYPE &&
		shaderNames[i].size() != 0; ++i) {
		ShaderType curType = static_cast<ShaderType>(i);
		const ShaderInfo& shader = analyseShader(shaderNames[i].c_str(), curType);
		/** 分析资源签名 */
		for (const auto& vSig : shader.signatures) {
			uint32_t idx = 0;
			ProgramInfo::EncodeReigsterIndex(idx, vSig.second.registerIndex);
			ProgramInfo::EncodeSpaceIndex(idx, vSig.second.spaceIndex);
			switch (vSig.second.type) {
			/** TODO: 暂时还没有支持UAV */
			case D3D_SIT_CBUFFER:
				signature.insert(std::make_pair(vSig.first, Parameter::PARAMETER_TYPE_BUFFER));
				ProgramInfo::EncodeType(idx, 1u);
				name_srv_cbv_uav_ptr.insert(std::make_pair(vSig.first, listInsert(srv_cbv_uav_idx, idx)));
				break;
			case D3D_SIT_TEXTURE:
				signature.insert(std::make_pair(vSig.first, Parameter::PARAMETER_TYPE_TEXTURE));
				name_srv_cbv_uav_ptr.insert(std::make_pair(vSig.first, listInsert(srv_cbv_uav_idx, idx)));
				break;
			case D3D_SIT_SAMPLER:
				signature.insert(std::make_pair(vSig.first, Parameter::PARAMETER_TYPE_SAMPLER));
				name_sampler_ptr.insert(std::make_pair(vSig.first, listInsert(sampler_idx, idx)));
				break;
			default:
				FLOG("%s: Doesn't support parameter: %s\n", __FUNCTION__, vSig.first.c_str());
			}
		}
		/** 校验IA的输入是否匹配 */
		if (shader.Type() == SHADER_TYPE_VERTEX_SHADER) {
			const InputLayoutSignatureEncode& ILSE = m_inputlayouts.find(va_handle)->second;
			if (VS_IO_ExceedMatch(ILSE.VS_IO, shader.VS_IO) == false) {
				FLOG("%s: IA Stage setting doesn't match this program\n", __FUNCTION__);
				return ProgramDescriptor::CreateInvalidDescirptor();
			}
			newProgramInfo.inputLayoutPtr = &ILSE.layout;
			/** 将IA的签名合并到整个签名中 */
			signature.insert(ILSE.inputLayoutSignature.begin(), ILSE.inputLayoutSignature.end());
		}
		/** 设置输出接口 */
		else if (shader.Type() == SHADER_TYPE_PIXEL_SHADER) {
			std::string semantic = "COLOR";
			for (uint8_t target = 0; target < getSV_TARGET(shader.PS_IO); ++target) {
				signature.insert(std::make_pair(semantic + std::to_string(target), Parameter::PARAMETER_TYPE_TEXTURE));
			}
			semantic = "DEPTH";
			for (uint8_t depth = 0; depth < getSV_DEPTH(shader.PS_IO); ++depth) {
				signature.insert(std::make_pair(semantic + std::to_string(depth), Parameter::PARAMETER_TYPE_TEXTURE));
			}
		}
	}
	{
		/** 构建Descriptor列表 */
		std::list<uint32_t>::iterator iter;
		iter = srv_cbv_uav_idx.begin();
		for (int i = 0; i < srv_cbv_uav_idx.size(); ++i, ++iter) {
			(*iter) += i;
		}
		for (const auto& e : name_srv_cbv_uav_ptr) {
			newProgramInfo.srv_cbv_uav_Desc.insert(std::make_pair(e.first, *e.second));
		}
		iter = sampler_idx.begin();
		for (int i = 0;	i < sampler_idx.size(); ++i, ++iter) {
			(*iter) += i;
		}
		for (const auto& e : name_sampler_ptr) {
			newProgramInfo.sampler_Desc.insert(std::make_pair(e.first, *e.second));
		}
	}
	/** 假如使用索引，需要添加索引签名 */
	if (usedIndex) signature.insert(std::make_pair("IDXBUF", Parameter::PARAMETER_TYPE_BUFFER));
	ProgramHandle newProgramHandle(m_nextProgramHandle++);

	{
		std::lock_guard<decltype(m_programLock)> lg(m_programLock);
		m_programs.insert(std::make_pair(newProgramHandle, newProgramInfo));
	}

	return 	ProgramDescriptor(std::move(signature), shaderNames, newProgramHandle,
		newProgramType, usedIndex, rasterization, outputStage);
}

const DX12RenderBackend::ProgramInfo & DX12RenderBackend::AccessProgramInfo(const ProgramHandle & handle) const
{
	std::lock_guard<OptimisticLock> lg(m_programLock);
	auto res = m_programs.find(handle);
	assert(res != m_programs.end());
	return res->second;
}

const DX12RenderBackend::ShaderInfo & DX12RenderBackend::AccessShaderInfo(const std::string & shaderName) const
{
	std::lock_guard<OptimisticLock> lg(m_shaderLock);
	auto iter = m_shaders.find(shaderName);
	assert(iter != m_shaders.end());
	return iter->second;
}

auto DX12RenderBackend::analyseShader(const char * shaderName, ShaderType type)
	-> const DX12RenderBackend::ShaderInfo&
{
	decltype(m_shaders)::iterator iterator;
	/** 判断shader是否已经被缓存 */
	{
		std::lock_guard<OptimisticLock> lg(m_shaderLock); /**< 考虑锁占用时间短，使用乐观锁 */
		iterator = m_shaders.find(shaderName);
		if (iterator != m_shaders.end()) {
			FLOG("%s has been exist!\n", shaderName);
		}
		else {
			auto [tempitr, res] = m_shaders.insert(std::make_pair(shaderName, ShaderInfo()));
			assert(res == true);
			iterator = tempitr;
		}
	}
	ShaderInfo& shader = iterator->second;
	DXCompilerHelper dxc;
	auto[timestamp, isExist] = dxc.TimeStampOfShaderSourceCode(shaderName);
	assert(isExist);
	if (shader.timestamp <= timestamp) {
		/** 缓存的字节码失效，需要重新加载 */
		/** 清除原有的shader缓存数据 */
		shader.Reset();
		{
			decltype(m_mapOfShaderAnalysationLock)::iterator lockIter;
			/** 获得shader文件锁 */
			{
				std::lock_guard<OptimisticLock> lg(m_shaderAnalysationLock);
				lockIter = m_mapOfShaderAnalysationLock.find(shaderName);
				if (lockIter == m_mapOfShaderAnalysationLock.end()) {
					/** 对应的锁不存在，创建锁 */
					auto[tempIter, res] = m_mapOfShaderAnalysationLock.insert(std::make_pair(shaderName,
						std::make_unique<std::mutex>()));
					assert(res == true);
					lockIter = tempIter;
				}
			}
			/** 加载新的blob */
			{
				std::lock_guard<std::mutex> lg(*(lockIter->second));
				/** 根据shader类型构造profile */
				switch (type) {
				case SHADER_TYPE_VERTEX_SHADER:
					shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(vs));
					break;
				case SHADER_TYPE_PIXEL_SHADER:
					shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(ps));
					break;
				case SHADER_TYPE_COMPUTE_SHADER:
					shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(cs));
					break;
				case SHADER_TYPE_GEOMETRY_SHADER:
					shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(gs));
					break;
				case SHADER_TYPE_TESSELLATION_SHADER:
					shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(ds));
					break;
				case SHADER_TYPE_HULL_SHADER:
					shader.blob = dxc.LoadShader(shaderName, SHADER_MODEL(hs));
					break;
				default:
					assert(false);
				}
				if (shader.blob == nullptr) {
					MLOG(dxc.LastErrorMsg());
					assert(false);
				}
			}
		}
		shader.timestamp = timestamp + 1; /**< 更新时间戳 */
		/** 对字节码重新进行分析 */
		SmartPTR<ID3D12ShaderReflection> ref = dxc.RetrieveShaderDescriptionFromByteCode(shader.blob);
		D3D12_SHADER_DESC shaderDesc;
		ref->GetDesc(&shaderDesc);
		shader.version = shaderDesc.Version;
		/** TODO: 支持更多类型shader的semantic */
		if (type == SHADER_TYPE_VERTEX_SHADER) {
			for (unsigned int i = 0; i < shaderDesc.InputParameters; ++i) {
				D3D12_SIGNATURE_PARAMETER_DESC sigParaDesc;
				ref->GetInputParameterDesc(i, &sigParaDesc);
				if (strcmp(sigParaDesc.SemanticName, "POSITION") == 0) {
					if (sigParaDesc.SemanticIndex >= getPOSITIONN(shader.VS_IO))
						setPOSITIONN(shader.VS_IO, sigParaDesc.SemanticIndex + 1);
				}
				else if (strcmp(sigParaDesc.SemanticName, "TEXCOORD") == 0) {
					if (sigParaDesc.SemanticIndex >= getTEXCOORDN(shader.VS_IO))
						setTEXCOORDN(shader.VS_IO, sigParaDesc.SemanticIndex + 1);
				}
				else if (strcmp(sigParaDesc.SemanticName, "NORMAL") == 0) {
					if (sigParaDesc.SemanticIndex >= getNORMALN(shader.VS_IO))
						setNORMALN(shader.VS_IO, sigParaDesc.SemanticIndex + 1);
				}
				else if (strcmp(sigParaDesc.SemanticName, "TANGENT") == 0) {
					if (sigParaDesc.SemanticIndex >= getTANGENTN(shader.VS_IO))
						setTANGENTN(shader.VS_IO, sigParaDesc.SemanticIndex + 1);
				}
				else {
					FLOG("%s: Doesn't support %s currently\n", __FUNCTION__, sigParaDesc.SemanticName);
				}
			}
		}
		else if (type == SHADER_TYPE_PIXEL_SHADER) {
			for (unsigned int i = 0; i < shaderDesc.OutputParameters; ++i) {
				D3D12_SIGNATURE_PARAMETER_DESC sigParaDesc;
				ref->GetOutputParameterDesc(i, &sigParaDesc);
				if (strcmp(sigParaDesc.SemanticName, "SV_TARGET") == 0) {
					if (sigParaDesc.SemanticIndex >= getSV_TARGET(shader.PS_IO))
						setSV_TARGET(shader.PS_IO, sigParaDesc.SemanticIndex + 1);
				}
				else if (strcmp(sigParaDesc.SemanticName, "SV_DEPTH") == 0) {
					if (sigParaDesc.SemanticIndex >= getSV_DEPTH(shader.PS_IO))
						setSV_DEPTH(shader.PS_IO, sigParaDesc.SemanticIndex + 1);
				}
			}
		}
		/** 分析constant buffer, texture以及sampler */
		for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
			D3D12_SHADER_INPUT_BIND_DESC resDesc;
			ref->GetResourceBindingDesc(i, &resDesc);

			shader.signatures.insert(std::make_pair(resDesc.Name,
				ShaderInfo::ResourceInfo(resDesc.Type, resDesc.BindPoint, resDesc.Space)));
		}
	}
	return shader;
}


auto DX12RenderBackend::analyseInputLayout(const VertexAttributeDescs & descs)
->InputLayoutSignatureEncode
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> res;
	std::map<std::string, Parameter::Type> sig;
	res.reserve(descs.size());
	uint64_t VS_IO = 0;
	std::string semantic = "VTXBUF";
	for (const auto& desc : descs) {
		D3D12_INPUT_ELEMENT_DESC dxDesc;
		dxDesc.InstanceDataStepRate = 0;
		dxDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		dxDesc.Format = ElementFormatToDXGIFormat(desc.format);
		dxDesc.InputSlot = desc.location;
		sig.insert(std::make_pair(semantic + std::to_string(desc.location), Parameter::PARAMETER_TYPE_BUFFER));
		dxDesc.SemanticIndex = desc.index;
		dxDesc.SemanticName = VertexAttributeTypeToString(desc.vertexAttribute);
		dxDesc.AlignedByteOffset = desc.byteOffset == 0 ? D3D12_APPEND_ALIGNED_ELEMENT : desc.byteOffset;
		res.emplace_back(dxDesc);
		switch (desc.vertexAttribute) {
		case VERTEX_ATTRIBUTE_TYPE_POSITION:
			if (desc.index >= getPOSITIONN(VS_IO)) setPOSITIONN(VS_IO, desc.index + 1); /**< 真实数量 = 索引 + 1 */
			break;
		case VERTEX_ATTRIBUTE_TYPE_NORMAL:
			if (desc.index >= getNORMALN(VS_IO)) setNORMALN(VS_IO, desc.index + 1);
			break;
		case VERTEX_ATTRIBUTE_TYPE_TANGENT:
			if (desc.index >= getTANGENTN(VS_IO)) setTANGENTN(VS_IO, desc.index + 1);
			break;
		case VERTEX_ATTRIBUTE_TYPE_TEXTURE:
			if (desc.index >= getTEXCOORDN(VS_IO)) setTEXCOORDN(VS_IO, desc.index + 1);
			break;
		default:
			FLOG("%s: Currently Doesn't support this attribute type\n", __FUNCTION__);
		}
	}
	return InputLayoutSignatureEncode(std::move(res), std::move(sig), VS_IO);
}

END_NAME_SPACE
