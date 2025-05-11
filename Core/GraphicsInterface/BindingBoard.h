#pragma once
#include "../UVConfig.h"
#include "../UVType.h"
#include <vector>
BEG_NAME_SPACE

class GPUResource;

/** 
* 这是一个DX12的概念，可以理解为是参数集的意思。我以DX12的视角解释下这个类型的含义
* 一个PSO可以由多个不同类型的Shader构成，它们各自所需要的参数又共同组成了这个PSO的RootSIgnature
* 一般来说，一个Shader里面的参数无外乎是c0，t0这些Buffer或者Texture的组合
* 因此，可以构建一个或者多个"参数集"，去描述这些c0，t0的组合方式。
* 比方说，一个PSO是VS + PS的组合，它们需要的参数这样的：
* | VS: c0 | PS: t0 |
* 那么这个RootSignature就可以由两个参数集(RootParameterTable)组成，它们分别是：
* RootParameterTable 0，包含c0
* RootParameterTable 1，包含t0
* 当我们往这个PSO绑定参数的时候，就需要对应绑定两个BindingBoard
* 一个BindingBoard 0中，slot 0上绑定一个constant buffer
* 一个BindingBoard 1中，slot 0上绑定一个Texture
* 之后BindingBoard 0绑定到PSO的RootParameterTable 0，BindingBoard 1绑定到PSO的RootParameterTable 1上
* 
* 当VS/PS配置了多个参数时，它们也可以被划分成多个RootParameterTable。它的意义在于：一些通用的参数及其绑定方式可以在多个PSO之间共享！
* 
* 就DX12而言，可以简单理解为一个BindingBoard对应一个RootParameterTable，无特殊情况下通常一个Shader就一个RootParameterTable
*/
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
