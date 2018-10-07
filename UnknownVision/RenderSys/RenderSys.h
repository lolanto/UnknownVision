#pragma once
#include "RenderSys_UVConfig.h"

// 渲染系统的抽象基类，声明了渲染系统的接口
class RenderSys {
public:
	RenderSys(API_TYPE api) : API(api) {}
	API_TYPE API;
public:
	// Ultility
	virtual void ResetAll() = 0;
	virtual void ClearAllBindingState() = 0;
	// For Shader Resource View
	virtual bool BindTexture() = 0; // tx
	virtual bool UnbindTexture() = 0;

	virtual bool BindConstBuffer() = 0; // cx
	virtual bool UnbindConstBuffer() = 0;

	virtual bool BindUnorderAccessData() = 0; // ux
	virtual bool UnbindUnorderAccessData() = 0;

	virtual bool BindSampler() = 0; // sx
	virtual bool UnbindSampler() = 0;

	// For Shaders
	virtual bool BindVertexShader() = 0;
	virtual bool UnbindVertexShader() = 0;

	virtual bool BindPixelShader() = 0;
	virtual bool UnbindPixelShader() = 0;

	virtual bool BindGeometryShader() = 0;
	virtual bool UnbindGeometryShader() = 0;

	virtual bool BindComputeShader() = 0;
	virtual bool UnbindComputeShader() = 0;

	// For Pipeline State
	virtual bool BindVertexBuffer() = 0;
	virtual bool UnbindVertexBuffer() = 0;

	virtual bool BindRenderTarget() = 0;
	virtual bool UnbindRenderTarget() = 0;

	virtual bool BindDepthStencilBuffer() = 0;
	virtual bool UnbindDepthStencilBuffer() = 0;

	virtual bool SetInputLayout() = 0;
	virtual bool SetCullMode() = 0;
	virtual bool SetPrimitiveType() = 0;
	// Draw Call
	virtual void DrawIndex() = 0;
	virtual void Draw() = 0;
};