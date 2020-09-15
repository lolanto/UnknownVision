#pragma once
#include "../UVConfig.h"
#include "../UVType.h"
#include <vector>
BEG_NAME_SPACE

class GPUResource;

class BindingBoard {
	friend class DX12RenderDevice;
public:
	struct SocketDesc {
		GPUResource* ptr;
		ShaderParameterType type;
	};
public:
	BindingBoard() : m_enableBinding(true) {}
	virtual ~BindingBoard() = default;
public:
	virtual void BindingResource(size_t slotIdx, GPUResource* ptr, ShaderParameterType type, ShaderParameterFlag flag1 = SHADER_PARAMETER_FLAG_NONE, int flag2 = 0) {}
	/** 当前bindingBord不再允许绑定resource，即BindingResource方法不再可用 */
	virtual void Close() { m_enableBinding = false; }
	/** 可以重新使用BindingResource */
	virtual void Reset() { m_enableBinding = true; }
	virtual size_t Capacity() const { return 0; }
	bool BindingState() const { return m_enableBinding; }
protected:
	COMMAND_UNIT_TYPE m_type;
	bool m_enableBinding;
};

END_NAME_SPACE
