#pragma once
#include "DX12Config.h"
#include "../GraphicsInterface/RenderBackend.h"
BEG_NAME_SPACE
class DX12RenderDevice;
class BasicShader;
class DX12RenderBackend : public RenderBackend {
public:
	static DX12RenderBackend* Get();
private:
	DX12RenderBackend() : m_isInitialized(false), m_factory(nullptr), m_devices({}), RenderBackend() {}
public:
	virtual ~DX12RenderBackend();
public:
	virtual bool Initialize() override final;
	virtual bool isInitialized() const override final { return m_isInitialized; }
public:
	RenderDevice* CreateDevice(void* parameters) final;
	RenderDevice* GetDevice(size_t node) override final;
	/** 完成Shader对象的初始化工作，包括编译，反射信息提取 */
	bool InitializeShaderObject(BasicShader* shader) final;

private:
	bool m_isInitialized;
	SmartPTR<IDXGIFactory6> m_factory;
	std::vector<DX12RenderDevice*> m_devices;
	
};

END_NAME_SPACE
