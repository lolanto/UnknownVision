#ifndef RENDER_SYS_H
#define RENDER_SYS_H

#include "../UVConfig.h"
#include <functional>
namespace UnknownVision {
	// 渲染系统的抽象基类，声明了渲染系统的接口
	class RenderSys {
	public:
		RenderSys(API_TYPE api, float width, float height) :
			m_basicHeight(height), m_basicWidth(width), API(api) {}
		const API_TYPE API;
	public:
		// Ultility
		virtual bool Init() = 0;
		void Run(std::function<void()>& func) { func(); }
		virtual void ResetAll() = 0;
		virtual void ClearAllBindingState() = 0;
		// For Shader Resource View
		//virtual bool BindTexture() = 0; // tx
		//virtual bool UnbindTexture() = 0;

		//virtual bool BindUnorderAccessData() = 0; // ux
		//virtual bool UnbindUnorderAccessData() = 0;

		//virtual bool BindSampler() = 0; // sx
		//virtual bool UnbindSampler() = 0;

		// For Shaders
		virtual bool BindShader(uint32_t index) = 0;
		virtual bool UnbindShader(uint32_t index) = 0;

		// For Pipeline State
		//virtual bool BindBuffer(Buffer&) = 0;
		//virtual bool UnbindBuffer(Buffer&) = 0;

		virtual bool BindVertexBuffer(uint32_t index) = 0;
		
		// 若index==-1 意味着使用backbuffer为RTV
		virtual bool BindRenderTarget(int index) = 0;
		virtual bool UnbindRenderTarget(int index) = 0;

		//virtual bool SetInputLayout() = 0;
		//virtual bool SetCullMode() = 0;
		virtual bool SetPrimitiveType() = 0;
		// Draw Call
		virtual void DrawIndex() = 0;
		virtual void Draw() = 0;
	protected:
		float m_basicWidth = 0;
		float m_basicHeight = 0;
	};
}

#endif // RENDER_SYS_H
