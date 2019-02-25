#include "LuaEngine.h"
#include "../Utility/InfoLog/InfoLog.h"
#include <lua.hpp>

namespace UnknownVision {

	LuaEngine::~LuaEngine() {
		if (m_state) lua_close(m_state);
	}

	bool LuaEngine::Initialize() {
		m_state = luaL_newstate();
		luaL_openlibs(m_state);
		/** 初始化引擎中的函数 */
		if (!setupFunctions()) {
			MLOG(LW, __FUNCTION__, LL, " initialize functions failed!");
			return false;
		}
		return true;
	}
}
