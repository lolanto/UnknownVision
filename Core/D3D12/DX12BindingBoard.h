#include "../GraphicsInterface/BindingBoard.h"
#include "DX12DescriptorHeap.h"
#include "DX12Config.h"
#include <d3d12.h>
BEG_NAME_SPACE
class DX12RenderDevice;
class DX12BindingBoard : public BindingBoard {
	friend class DX12RenderDevice;
public:
	virtual ~DX12BindingBoard() = default;
	DX12BindingBoard() = default;
	virtual void BindingResource(size_t slotIdx, GPUResource* ptr, ShaderParameterType type, ShaderParameterFlag flag1 = SHADER_PARAMETER_FLAG_NONE, int flag2 = 0) override final;
	virtual size_t Capacity() const override final;
	virtual void Close() override final;
	virtual void Reset() override final;
public:
	/** 仅子类拥有的接口 */
	void Initialize(size_t capacity, DX12RenderDevice* pDevice, COMMAND_UNIT_TYPE type);
	void SetLastFenceValue(size_t newFenceValue) { m_lastFenceValue = newFenceValue; }
	AllocateRange AllocateDescriptorHeap() const { return m_alloc; }
private:
	AllocateRange m_alloc;
	size_t m_lastFenceValue;
	BasicDX12DescriptorHeap m_cpuHeap;
	DX12RenderDevice* m_pDevice;
};

END_NAME_SPACE
