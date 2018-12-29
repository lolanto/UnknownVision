#ifndef RENDER_SYS_H
#define RENDER_SYS_H

#include "../UVConfig.h"
#include "../ResMgr/IResMgr.h"
#include "./PipelineStateDesc.h"
#include <functional>
namespace UnknownVision {
	enum PipelineStage {
		PS_InputAssemble,
		PS_VertexProcess,
		PS_GeometryProcess,
		PS_TesselationProcess,
		PS_PixelProcess,
		PS_BlendProcess
	};
	enum Primitive {
		PRI_Point,
		PRI_Triangle
	};
	// 渲染系统的抽象基类，声明了渲染系统的接口
	class RenderSys {
	public:
		RenderSys(API_TYPE api, float width, float height) :
			m_basicHeight(height), m_basicWidth(width), API(api) {}
		virtual ~RenderSys() = default;
		float Width() const { return m_basicWidth; }
		float Height() const { return m_basicHeight; }
		const API_TYPE API;
	public:
		// Ultility
		virtual bool Init() = 0;
		virtual void Run(std::function<void()>&& func) = 0;
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
		virtual bool UnbindShader(ShaderType type) = 0;

		// For Pipeline State
		//virtual bool BindBuffer(Buffer&) = 0;
		//virtual bool UnbindBuffer(Buffer&) = 0;
		// 创建输入格式
		virtual int CreateInputLayout(std::vector<SubVertexAttributeLayoutDesc>& descs,
			int vertexShader = -1) = 0;
		// 激活某个输入格式
		virtual bool ActiveInputLayout(uint32_t index) = 0;

		virtual int CreateViewPort(const ViewPortDesc& desc) = 0;
		virtual bool ActiveViewPort(uint32_t index) = 0;

		virtual bool BindVertexBuffer(uint32_t index) = 0;
		virtual bool BindVertexBuffers(uint32_t* indices, size_t numBuf) = 0;
		virtual bool BindConstantBuffer(uint32_t index, PipelineStage stage, uint32_t slot) = 0;
		
		virtual bool BindDepthStencilTarget(uint32_t index) = 0;
		virtual void UnbindDepthStencilTarget() = 0;
		// 若index==-1 意味着使用backbuffer为RTV
		virtual bool BindRenderTarget(int index) = 0;
		virtual void UnbindRenderTarget() = 0;
		// 清空RTV
		virtual void ClearRenderTarget(int index) = 0;

		//virtual bool SetInputLayout() = 0;
		//virtual bool SetCullMode() = 0;
		// 设置图元类型
		virtual bool SetPrimitiveType(Primitive pri) = 0;
		// Draw Call
		virtual void DrawIndex() = 0;
		virtual void Draw() = 0;
		virtual void Present() = 0;

		ShaderMgr& ShaderManager() const { return *m_shaderMgr.get(); }
		BufferMgr& BufferManager() const { return *m_bufMgr.get(); }
		Texture2DMgr& Texture2DManager() const { return *m_tex2DMgr.get(); }

	protected:
		float m_basicWidth = 0;
		float m_basicHeight = 0;
		std::unique_ptr<ShaderMgr> m_shaderMgr;
		std::unique_ptr<BufferMgr> m_bufMgr;
		std::unique_ptr<Texture2DMgr> m_tex2DMgr;
	};
}

#endif // RENDER_SYS_H
