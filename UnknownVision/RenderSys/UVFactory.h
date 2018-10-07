#pragma once
#include "RenderSys_UVConfig.h"
#include "RenderSys.h"
#include "ResourceManager.h"
#include "../Utility/MainClass/MainClass.h"

#include <memory>

class UVFactory {
// standalone
public:
	static UVFactory& GetInstance();
private:
	UVFactory() : m_hasInit(false) {};

public:
	bool Init(API_TYPE apiType, MainClass& mc);
	std::shared_ptr<RenderSys> GetFactory() { return m_renderSys; };
private:
	bool createDX11RenderSys(HWND hwnd,
		float winWidth, float winHeight);

private:
	API_TYPE										m_apiType;
	bool												m_hasInit;
	std::shared_ptr<RenderSys>			m_renderSys;
	std::shared_ptr<CanvasManager> m_canvasMgr;
	std::shared_ptr<ShaderManager> m_shaderMgr;
};
