#ifndef UV_FACTORY_H
#define UV_FACTORY_H
#include "../UVConfig.h"
#include "RenderSys.h"

#include <memory>
namespace UnknownVision {
	class UVFactory {
		// standalone
	public:
		static UVFactory& GetInstance() {
			static UVFactory _ins;
			return _ins;
		}
	private:
		UVFactory() : m_hasInit(false) {};

	public:
		bool Init(API_TYPE api, float width, float height) {
			switch (api) {
			case DirectX11_0:
				return createDX11RenderSys(m_renderSys, api, width, height);
			}
			return false;
		}
		RenderSys & GetRenderSys() const { return *m_renderSys; }
	private:
		bool createDX11RenderSys(std::unique_ptr<RenderSys>& rs, API_TYPE api, float width, float height);

	private:
		API_TYPE										m_apiType;
		bool												m_hasInit;
		std::unique_ptr<RenderSys>			m_renderSys;
	};
}

#endif // UV_FACTORY_H
