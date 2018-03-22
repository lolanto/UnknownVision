#pragma once
#include <DirectXMath.h>
#include <Windows.h>
#include <memory>

typedef unsigned int UINT;

class MeshManager;
class IRenderTarget;
class ITexture;
class UObject;
struct ID3D11RenderTargetView;

enum ShaderResourceViewTarget {
	SRVT_VERTEX_SHADER = 0,
	SRVT_PIXEL_SHADER
};

struct ShaderResourcesDesc {
	ITexture** resources;
	size_t num;
	UINT* slots;
	ShaderResourceViewTarget* targets;
};

struct RenderTargetsDesc {
	IRenderTarget** targets;
	size_t num;
	bool* clear;
	// unset!
	std::shared_ptr<ID3D11RenderTargetView*> data;
	RenderTargetsDesc() : data(NULL) {}
};

class RendererProxy {
public:
	virtual bool InitSys(HWND, float, float) = 0;
public:

	virtual void Setup(UObject*) = 0;

	virtual void Bind(UObject*) = 0;
	virtual void Bind(ShaderResourcesDesc*) = 0;
	virtual void Bind(RenderTargetsDesc*, bool, bool) = 0;

	virtual void Unbind(UObject*) = 0;
	virtual void Unbind(ShaderResourcesDesc*) = 0;
	virtual void Unbind(RenderTargetsDesc*) = 0;

	virtual MeshManager* GetMeshManager() = 0;
};