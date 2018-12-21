﻿#pragma once
#include "RenderSys_UVConfig.h"
class Shader;
class Buffer;
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

	virtual bool BindUnorderAccessData() = 0; // ux
	virtual bool UnbindUnorderAccessData() = 0;

	virtual bool BindSampler() = 0; // sx
	virtual bool UnbindSampler() = 0;

	// For Shaders
	virtual bool BindShader(Shader&) = 0;
	virtual bool UnbindShader(Shader&) = 0;

	// For Pipeline State
	virtual bool BindBuffer(Buffer&) = 0;
	virtual bool UnbindBuffer(Buffer&) = 0;

	virtual bool BindRenderTarget() = 0;
	virtual bool UnbindRenderTarget() = 0;

	virtual bool SetInputLayout() = 0;
	virtual bool SetCullMode() = 0;
	virtual bool SetPrimitiveType() = 0;
	// Draw Call
	virtual void DrawIndex() = 0;
	virtual void Draw() = 0;
};