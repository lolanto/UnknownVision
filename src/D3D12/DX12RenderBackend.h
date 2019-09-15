#pragma once
#include "DX12Config.h"
#include "../RenderSystem/RenderBackend.h"
#include <assert.h>
BEG_NAME_SPACE
class DX12RenderDevice;

class DX12RenderBackend : public RenderBackend {
public:

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

public:
	~DX12RenderBackend() {
		for (auto devPtr : m_devices)
			delete devPtr;
	}
public:
	bool Initialize() final;
	bool isInitialized() const final { return m_isInitialized; }
	bool InitializeShaderObject(ShaderInterface* shader) final;
	RenderDevice* CreateDevice(void* parameters) final;

	/** 注册顶点结构描述，以便重复使用 */
	VertexAttributeHandle RegisterVertexAttributeDescs(const VertexAttributeDescs& descs) final thread_safe {
		VertexAttributeHandle handle(m_nextVertexAttributeHandle++);
		std::lock_guard<OptimisticLock> lg(m_inputLayoutLock);
		auto[iter, res] = m_inputlayouts.insert(std::make_pair(handle, analyseInputLayout(descs)));
		if (res == false) return VertexAttributeHandle::InvalidIndex();
		else return handle;
	}
public:
	/** 透过VertexAttributeHandle访问注册的DX12Input Layout */
	const InputLayoutSignatureEncode& AccessVertexAttributeDescs(const VertexAttributeHandle& handle) thread_safe_const {
		std::lock_guard<OptimisticLock> lg(m_inputLayoutLock);
		auto inputLayout = m_inputlayouts.find(handle);
		assert(inputLayout != m_inputlayouts.end());
		return inputLayout->second;
	}

private:

	/** 将顶点属性描述信息转换成DX12可以解析的结构 */
	auto analyseInputLayout(const VertexAttributeDescs & descs)
		->InputLayoutSignatureEncode;

private:
	bool m_isInitialized = false;
	SmartPTR<IDXGIFactory6> m_factory;
	std::vector<DX12RenderDevice*> m_devices;

	mutable OptimisticLock m_inputLayoutLock;
	std::map<VertexAttributeHandle, InputLayoutSignatureEncode> m_inputlayouts;
};

END_NAME_SPACE
