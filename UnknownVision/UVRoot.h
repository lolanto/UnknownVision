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
		bool Init(API_TYPE api, float width, float height) {
			switch (api) {
			case DirectX11_0:
				return createDX11Env(api, width, height);
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
		bool createDX11Env(API_TYPE api, float width, float height);

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
