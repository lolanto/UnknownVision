#pragma once

#include "../RenderSystem/RenderBasic.h"
#include "DX12Config.h"
#include "DX12Helpers.h"
#include "DX12ResourceManager.h"
#include <vector>
#include <memory>
#include <array>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>

#define MemoryManagementStrategy NoMemMng

BEG_NAME_SPACE

/** 分析输出阶段中关于blending过程的内容，并生成blending设置 */
D3D12_BLEND_DESC AnalyseBlendingOptionsFromOutputStageOptions(const OutputStageOptions& osOpt);
/** 分析输出阶段中关于Depth和stencil过程的设置，并生成Depth Stencil设置 */
D3D12_DEPTH_STENCIL_DESC AnalyseDepthStencilOptionsFromOutputStageOptions(const OutputStageOptions& osOpt);
/** 分析光栅化的设置并生成DX12相应的设置 */
D3D12_RASTERIZER_DESC AnalyseRasterizerOptionsFromRasterizeOptions(const RasterizeOptions& rastOpt);

class DX12RenderBackend;
class DX12RenderDevice;


enum CmdQueueType {
	GraphicQue = 0,
	CmdQueueCount
};


class BufMgr {
public:
	using ElementType = SmartPTR<ID3D12Resource>;
	using IndexType = BufferHandle;
private:
	std::deque<ElementType> buffers; /**< 当前正在管理的所有buffer资源 */
	std::vector<IndexType> freeIndices; /**< 空闲buffer句柄 */
	mutable std::mutex bm_mutex;
public:
	IndexType RequestBufferHandle() thread_safe;
	void RevertBufferHandle(IndexType index) thread_safe;
	void SetBuffer(ElementType&& newElement, IndexType index) thread_safe;
	const ElementType& operator[](const IndexType& index) thread_safe_const;
	ElementType& operator[](const IndexType& index) thread_safe;
};


class TransientResourceMgr {
public:
	using ElementType = SmartPTR<ID3D12Resource>;
private:
	std::vector<ElementType> resources;
	std::mutex trm_mutex;
public:
	void Store(ElementType& e) thread_safe;
	void Reset() { resources.swap(std::vector<ElementType>()); }
};

class CommandListMgr {
public:
	using ElementType = SmartPTR<ID3D12GraphicsCommandList>;
	CommandListMgr(ID3D12Device* dev) { dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)); }
private:
	SmartPTR<ID3D12CommandAllocator> allocator;
	std::vector<ElementType> cmdLists;
	std::vector<uint8_t> freeCmdLists;
	mutable std::mutex clm_mutex;
public:
	ID3D12GraphicsCommandList* RequestCmdList() thread_safe;
	const std::vector<ElementType>& RequestWholeList() const { return cmdLists; }
	void SimpleRest() {
		freeCmdLists.resize(cmdLists.size());
		for (uint8_t i = 0; i < freeCmdLists.size(); ++i)
			freeCmdLists[i] = i;
	}
	void DeepRest() {
		cmdLists.clear();
		freeCmdLists.clear();
		allocator->Reset();
	}
};

class QueueProxy {
public:
	QueueProxy(SmartPTR<ID3D12CommandQueue>& que,
		SmartPTR<ID3D12Fence>& fen)
		: queue(que), fence(fen) {
		fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		fenceValue = 0;
	}
	QueueProxy(const QueueProxy&) = delete;
	QueueProxy& operator=(const QueueProxy&) = delete;
	QueueProxy(QueueProxy&& qp);
	QueueProxy& operator=(QueueProxy&& rhs);
	QueueProxy() : fenceEvent(NULL), fenceValue(0) {}
	~QueueProxy() { if (fenceEvent) CloseHandle(fenceEvent); }
	ID3D12Fence* GetFence() { return fence.Get(); }
	void Execute(uint32_t numCommandLists, ID3D12CommandList* const* ppCommandLists);
	void Wait(ID3D12Fence* fen, uint64_t value) { queue->Wait(fen, value); }
private:
	SmartPTR<ID3D12CommandQueue> queue;
	SmartPTR<ID3D12Fence> fence;
	HANDLE fenceEvent;
	uint64_t fenceValue;
};

class DX12RenderDevice : public RenderDevice {
	friend class DX12RenderBackend;
public:
	/** 单个句柄对应的实体的信息 */
	struct BufferInfo {
		BufferInfo(size_t size = 0, ID3D12Resource* ptr = nullptr,
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON)
			: ptr(ptr), state(state), size(size) {}
		ID3D12Resource* ptr = nullptr;
		D3D12_RESOURCE_STATES state;
		const size_t size = 0;
	};

	struct TextureInfo {
		TextureInfo(ID3D12Resource* ptr = nullptr, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON, 
			uint32_t width = 0, uint32_t height = 0)
			: ptr(ptr), state(state), width(width), height(height) {}
		ID3D12Resource* ptr = nullptr;
		D3D12_RESOURCE_STATES state;
		const uint32_t width = 0, height = 0;
	};

	struct SamplerInfo {
		D3D12_SAMPLER_DESC desc;
	};
public:
	bool Initialize(std::string config) final;

	/** 允许额外的线程执行该函数，负责处理任务队列中的任务 */
	void Process() final;

	ID3D12Device* GetDevice() { return m_device.Get(); }

private:
	/** 存储生成的PSO及其相应的rootSignature
	 * TODO: 暂时不考虑 rootSignature的重用问题 */
	struct PSOAndRootSig {
		SmartPTR<ID3D12PipelineState> pso;
		SmartPTR<ID3D12RootSignature> rootSignature;
	};
private:
	DX12RenderDevice(const DX12RenderBackend& backend,
		SmartPTR<ID3D12CommandQueue>& queue,
		SmartPTR<IDXGISwapChain1>& swapChain,
		SmartPTR<ID3D12Device>& device,
		uint32_t width, uint32_t height)
		: m_backend(backend), m_swapChain(swapChain), m_device(device), 
		m_backBuffers(decltype(m_backBuffers)(NUMBER_OF_BACK_BUFFERS)), m_curBackBufferIndex(0), RenderDevice(width, height),
		m_resourceManager(DX12ResourceManager(device.Get())) {
		SmartPTR<ID3D12Fence> fence;
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	}

	const DX12RenderDevice& operator=(const DX12RenderDevice&) = delete;
	DX12RenderDevice(DX12RenderDevice&&) = delete;
	DX12RenderDevice(const DX12RenderDevice&) = delete;

	void TEST_func(const Command&);
public:
	/** 根据program descriptor 创建GraphicsPipeline state object */
	bool generateGraphicsPSO(const ProgramDescriptor& pmgDesc);
	/** 根据program descriptor 创建该program的root signature */
	bool generateGraphicsRootSignature(const ProgramDescriptor& pmgDesc, SmartPTR<ID3D12RootSignature>& rootSignature);

private:
	const DX12RenderBackend& m_backend;
	SmartPTR<IDXGISwapChain1> m_swapChain;
	SmartPTR<ID3D12Device> m_device;
	std::vector< SmartPTR<ID3D12Resource> > m_backBuffers; /**< 需要手动构建队列 */
	DX12ResourceManager m_resourceManager; /**< 资源管理器 */
	/** 当前可以写入的后台缓存的索引，每一次切换frame buffer的时候都需要更新该值
	 * 取值范围是[0, BACK_BUFFER_COUNT] */
	uint8_t m_curBackBufferIndex = 0; 

	std::map<BufferHandle, BufferInfo> m_buffers; /**< 所有缓冲区句柄的信息都在这里 */
	std::map<TextureHandle, TextureInfo> m_textures; /**< 所有纹理句柄信息都在这里 */
	std::map<SamplerHandle, SamplerInfo> m_samplers; /**< 所有采样方式信息都在这里 */

	std::map<ProgramHandle, PSOAndRootSig> m_psoAndRootSig; /**< 每个程序对应pso, rootSignature存储于此 */

	std::atomic_uint64_t m_totalFrame;
};

inline uint8_t bit4Decode(const uint64_t& value, uint8_t pos) {
	return static_cast<uint8_t>((value >> (pos * 4)) & 0x000000000000000Fu);
}

inline void bit4Encode(uint64_t& value, uint8_t pos, uint8_t newValue) {
	uint64_t t1 = 0xFFu;
	t1 = t1 << (pos * 4);
	t1 = ~t1;
	value &= t1;
	t1 = newValue;
	t1 = t1 << (pos * 4);
	value |= t1;
}

#define getTEXCOORDN(VS_IO) bit4Decode(VS_IO, 0)
#define setTEXCOORDN(VS_IO, value) bit4Encode(VS_IO, 0, value)
#define getTANGENTN(VS_IO) bit4Decode(VS_IO, 1)
#define setTANGENTN(VS_IO, value) bit4Encode(VS_IO, 1, value)
#define getPSIZEN(VS_IO) bit4Decode(VS_IO, 2)
#define setPSIZEN(VS_IO, value) bit4Encode(VS_IO, 2, value)
#define getPOSITIONTN(VS_IO) bit4Decode(VS_IO, 3)
#define setPOSITIONTN(VS_IO, value) bit4Encode(VS_IO, 3, value)
#define getPOSITIONN(VS_IO) bit4Decode(VS_IO, 4)
#define setPOSITIONN(VS_IO, value) bit4Encode(VS_IO, 4, value)
#define getNORMALN(VS_IO) bit4Decode(VS_IO, 5)
#define setNORMALN(VS_IO, value) bit4Encode(VS_IO, 5, value)
#define getCOLORN(VS_IO) bit4Decode(VS_IO, 6)
#define setCOLORN(VS_IO, value) bit4Encode(VS_IO, 6, value)
#define getBLENDWEIGHTN(VS_IO) bit4Decode(VS_IO, 7)
#define setBLENDWEIGHTN(VS_IO, value) bit4Encode(VS_IO, 7, value)
#define getBLENDINDICESN(VS_IO) bit4Decode(VS_IO, 8)
#define setBLENDINDICESN(VS_IO, value) bit4Encode(VS_IO, 8, value)
#define getBINORMALN(VS_IO) bit4Decode(VS_IO, 9)
#define setBINORMALN(VS_IO, value) bit4Encode(VS_IO, 9, value)

#define getSV_DEPTH(PS_IO) bit4Decode(PS_IO, 0)
#define setSV_DEPTH(PS_IO, value) bit4Encode(PS_IO, 0, value)
#define getSV_TARGET(PS_IO) bit4Decode(PS_IO, 1)
#define setSV_TARGET(PS_IO, value) bit4Encode(PS_IO, 1, value)

/** 分析a是否“过度匹配”b */
inline bool VS_IO_ExceedMatch(uint64_t VS_IO_a, uint64_t VS_IO_b) {
	for (int i = 0; i < 10; ++i) {
		if ((VS_IO_a & 0x0Fu) < (VS_IO_b & 0x0Fu)) return false;
		VS_IO_a = VS_IO_a >> 4;
		VS_IO_b = VS_IO_b >> 4;
	}
	return true;
}

class DX12RenderBackend : public RenderBackend {
public:
	/** 用于存放加载的shader字节码和reflect的属性 */
	struct ShaderInfo {
		struct ResourceInfo {
			D3D_SHADER_INPUT_TYPE type;
			uint8_t registerIndex;
			uint8_t spaceIndex;
			/** TODO: 这个方式似乎不支持数组类型的资源 */
			ResourceInfo(D3D_SHADER_INPUT_TYPE type = D3D10_SIT_CBUFFER,
				uint8_t rId = 0, uint8_t sid = 0) : type(type), registerIndex(rId), spaceIndex(sid) {}
			ResourceInfo(const ResourceInfo& rhs) : type(rhs.type), registerIndex(rhs.registerIndex),
				spaceIndex(rhs.spaceIndex) {}
			ResourceInfo(ResourceInfo&& rhs) : ResourceInfo(rhs) {}
		};
		SmartPTR<ID3DBlob> blob;
		uint32_t version; /**< 包含该shader的版本信息，编码规则见https://docs.microsoft.com/zh-cn/windows/win32/api/d3d12shader/ns-d3d12shader-_d3d12_shader_desc */
		uint64_t timestamp;
		union {
			/** VS_IO记录了DX12中各个顶点属性是否被使用，使用的数量，编码规则为:
			 * 0-3: TEXCOORD; 4-7: TANGENT; 8-11: PSIZE; 12-15: POSITIONT; 16-19: POSITION
			 * 20-23: NORMAL; 24-27: COLOR; 28-31: BLENDWEIGHT; 32-35: BLENDINDICES; 36-39: BINORMAL*/
			uint64_t VS_IO;
			/** PS_IO记录了DX12中各个输出slot是否被使用，使用的数量，编码规则如下
			 * 0-3: SV_DEPTH; 4-7: SV_TARGET*/
			uint64_t PS_IO;
		};
		std::map<std::string, ResourceInfo> signatures;
		ShaderInfo() : timestamp(0), version(0), VS_IO(0), PS_IO(0) {}
		ShaderType Type() const {
			uint8_t type = (version & 0xFFFF0000) >> 16;
			switch (type) {
			case 0: return SHADER_TYPE_PIXEL_SHADER;
			case 1: return SHADER_TYPE_VERTEX_SHADER;
			case 2: return SHADER_TYPE_GEOMETRY_SHADER;
			case 5: return SHADER_TYPE_COMPUTE_SHADER;
			default:
				FLOG("%s: Currently doesn't support this shader type\n", __FUNCTION__);
				return SHADER_TYPE_NUMBER_OF_TYPE;
			}
		}
		uint8_t MajorVersion() const { return static_cast<uint8_t>((version & 0x000000F0) >> 4); }
		uint8_t MinorVersion() const { return static_cast<uint8_t>(version & 0x0000000F); }
		/** 清空原有内容 */
		void Reset() {
			blob.Reset();
			version = 0; timestamp = 0;
			signatures.clear();
		}
	};

	struct ProgramInfo {
		const std::vector<D3D12_INPUT_ELEMENT_DESC>* inputLayoutPtr;
		std::map<std::string, uint32_t> srv_cbv_uav_Desc; /**< 资源名称与其在descirptor heap中的索引映射关系 */
		std::map<std::string, uint32_t> sampler_Desc;
		static void EncodeHelper(uint32_t& idx, uint8_t offset, uint32_t mask, uint8_t value) {
			mask = mask << offset;
			mask = ~mask;
			idx &= mask;
			mask = value;
			mask = mask << offset;
			idx |= mask;
		}
		/** 向idx编码寄存器索引value */
		static void EncodeReigsterIndex(uint32_t& idx, uint8_t value) {
			EncodeHelper(idx, 12, 0xFFu, value);
		}
		/** 向idx编码空间索引value */
		static void EncodeSpaceIndex(uint32_t& idx, uint8_t value) {
			EncodeHelper(idx, 20, 0xFFu, value);
		}
		/** 向idx编码类型编号 SRV: 0x00, CBV: 0x01, UAV: 0x02 */
		static void EncodeType(uint32_t& idx, uint8_t type) {
			EncodeHelper(idx, 28, 0x0Fu, type);
		}
		static uint8_t DecodeRegisterIndex(const uint32_t& idx) { return (idx & 0x000FF000u) >> 12; }
		static uint8_t DecodeSpaceIndex(const uint32_t& idx) { return (idx & 0x0FF00000u) >> 20; }
		/** 仅对SRV CBV UAV有效 */
		static D3D12_DESCRIPTOR_RANGE_TYPE DecodeType(const uint32_t& idx) { 
			switch ((idx & 0xF0000000u) >> 28) {
			case 0: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			case 1: return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			case 2: return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			default:
				FLOG("%s: Invalid encode method! Unable to decode the type!\n", __FUNCTION__);
				assert(false);
			}
		}
		static uint32_t DecodeIndex(const uint32_t& idx) { return idx & 0x00000FFFu; }
	};

public:
	~DX12RenderBackend() {
		for (auto devPtr : m_devices)
			delete devPtr;
	}
public:
	bool Initialize() final;
	bool isInitialized() const final { return m_isInitialized; }
	RenderDevice* CreateDevice(void* parameters) final;
	ProgramDescriptor RequestProgram(const ShaderNames& shaderNames, VertexAttributeHandle va_handle,
		bool usedIndex, RasterizeOptions rasterization, OutputStageOptions outputStage) final;

	/** 注册顶点结构描述，以便重复使用 */
	VertexAttributeHandle RegisterVertexAttributeDescs(const VertexAttributeDescs& descs) final {
		VertexAttributeHandle handle(m_nextVertexAttributeHandle++);
		auto[iter, res] = m_inputlayouts.insert(std::make_pair(handle, analyseInputLayout(descs)));
		if (res == false) return VertexAttributeHandle::InvalidIndex();
		else return handle;
	}
public:
	/** 透过program handle访问程序信息 */
	const ProgramInfo& AccessProgramInfo(const ProgramHandle& handle) const;
	/** 透过shader名称访问shader信息
	 * @remark 调用该函数前务必确保已经调用过RequestProgram，否则会无法查找到shader*/
	const ShaderInfo& AccessShaderInfo(const std::string& shaderName) const;
private:
	/** 存储用户注册的顶点缓冲结构，并提前生成DX的Input layout和签名及其编码
	 * 方便之后使用 */
	struct InputLayoutSignatureEncode {
		std::vector<D3D12_INPUT_ELEMENT_DESC> layout; /**< 提前生成的DX input layout */
		/** 顶点缓冲签名只针对到缓冲，如VTXBUF0，VTXBUF1等。至于缓冲中的实际内容由用户控制 */
		std::map<std::string, Parameter::Type> inputLayoutSignature; /**< 根据用户提供的顶点缓冲结构生成的签名 */
		uint64_t VS_IO; /**< 签名编码后的结果，用于与关联的VS进行匹配校验 */

		InputLayoutSignatureEncode(std::vector<D3D12_INPUT_ELEMENT_DESC>&& layout,
			std::map<std::string, Parameter::Type>&& sig, uint64_t encode)
			: layout(layout), inputLayoutSignature(sig), VS_IO(encode) {}
		InputLayoutSignatureEncode() {}
		InputLayoutSignatureEncode(InputLayoutSignatureEncode&& rhs) : VS_IO(rhs.VS_IO) { layout.swap(rhs.layout); inputLayoutSignature.swap(rhs.inputLayoutSignature); }
	};
private:
	/** 解析shader的签名，假如字节码缓冲已经过期，更新字节码缓冲 */
	auto analyseShader(const char* shaderName, ShaderType type)
		-> const DX12RenderBackend::ShaderInfo&;

	/** 将顶点属性描述信息转换成DX12可以解析的结构 */
	auto analyseInputLayout(const VertexAttributeDescs & descs)
		->InputLayoutSignatureEncode;
	
private:
	bool m_isInitialized = false;
	SmartPTR<IDXGIFactory6> m_factory;
	std::vector<DX12RenderDevice*> m_devices;

	mutable OptimisticLock m_shaderLock;
	std::map<std::string, ShaderInfo> m_shaders;
	std::map<VertexAttributeHandle, InputLayoutSignatureEncode> m_inputlayouts;

	mutable OptimisticLock m_programLock;
	std::map<ProgramHandle, ProgramInfo> m_programs;
};

END_NAME_SPACE
