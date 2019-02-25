#ifndef UV_ROOT_H
#define UV_ROOT_H
#include "UVConfig.h"
#include "Utility/WindowBase/WindowBase.h"
#include <memory>
namespace UnknownVision {
	class RenderSys;
	class ShaderMgr;
	class BufferMgr;
	class Texture2DMgr;
	class VertexDeclarationMgr;
	class Root {
		// standalone
	public:
		inline static Root& GetInstance() {
			static Root _ins;
			return _ins;
		}
	private:
		Root() : m_hasInit(false) {};

	public:
		void Run() {
			if (m_window) m_window->Run();
		}
		/** 初始化整个系统
		 * @param api 渲染系统使用的API类型 
		 * @param width 渲染主窗口的宽度
		 * @param height 渲染主窗口的高度 */
		bool Init(API_TYPE api, uint32_t width, uint32_t height) {
			switch (api) {
			case DirectX11_0:
				return createDX11Env(api, width, height);
			case DirectX12_0:
				return createDX12Env(api, width, height);
			}
			return false;
		}
		RenderSys & GetRenderSys() const { return *m_renderSys; }
		ShaderMgr& GetShaderMgr() const { return *m_shaderMgr; }
		BufferMgr& GetBufferMgr() const { return *m_bufferMgr; }
		Texture2DMgr& GetTexture2DMgr() const { return *m_tex2dMgr; }
		VertexDeclarationMgr& GetVertexDeclarationMgr() const { return *m_vtxDeclMgr; }

		void SetWindow(WindowBase* ptr) {  }
	private:
		bool createDX11Env(API_TYPE api, uint32_t width, uint32_t height);
		bool createDX12Env(API_TYPE api, uint32_t width, uint32_t height);
	private:
		API_TYPE										m_apiType;
		bool												m_hasInit;
		std::unique_ptr<RenderSys>			m_renderSys;
		std::unique_ptr<ShaderMgr>		m_shaderMgr;
		std::unique_ptr<BufferMgr>			m_bufferMgr;
		std::unique_ptr<Texture2DMgr>	m_tex2dMgr;
		std::unique_ptr<VertexDeclarationMgr> m_vtxDeclMgr;
		std::unique_ptr<WindowBase>		m_window;
	};
}

#endif // UV_ROOT_H
