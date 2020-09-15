#include "LuaEngine.h"
#include "LuaFunctions.h"
#include "../../Utility/InfoLog/InfoLog.h"
#include <lua.hpp>
#include <iostream>
#include <map>

namespace UnknownVision {

	LuaEngine::~LuaEngine() {
		if (m_state) lua_close(m_state);
	}

	bool LuaEngine::Initialize() {
		m_state = luaL_newstate();
		luaL_openlibs(m_state);
		/** 初始化引擎中的函数 */
		if (!(setupFunctions() && setupGlobalVariable())) {
			char outputInfo[32] = { 0 };
			sprintf(outputInfo, "Function: %s FAILED", __FUNCTION__);
			MLOG(outputInfo);
			return false;
		}
		return true;
	}
	void LuaEngine::AnalyseScript(const char * mainFile)
	{
		if (0 != luaL_dofile(m_state, mainFile)) {
			std::cout << lua_tostring(m_state, -1);
		}
	}

	bool LuaEngine::setupFunctions() {
		lua_pushcfunction(m_state, CreateResource);
		lua_setglobal(m_state, "Resource");
		lua_pushcfunction(m_state, CreateBlock_U8);
		lua_setglobal(m_state, "Block_U8");
		return true;
	}

	bool LuaEngine::setupGlobalVariable() {
		static std::map<std::string, uint32_t> constantValueMap({
			{"BUFFER", 1}, {"TEXTURE2D", 2}
			});

		lua_newtable(m_state); /**< 代理table */
		luaL_newmetatable(m_state, "CONST_META"); /**< metatable */
		lua_pushcfunction(m_state, [](lua_State* L)->int {
			std::string key = lua_tostring(L, -1);
			const auto& iter = constantValueMap.find(key);
			if (iter == constantValueMap.end()) {
				luaL_error(L, "Not Such Key: %s", key.c_str());
				return 0;
			}
			else {
				lua_pushinteger(L, iter->second);
				return 1;
			}
		});
		lua_setfield(m_state, -2, "__index");
		lua_pushcfunction(m_state, [](lua_State* L)->int {
			luaL_error(L, "Can Not Modify Constant Value");
			return 0;
		});
		lua_setfield(m_state, -2, "__newindex");
		lua_setmetatable(m_state, -2);
		lua_setglobal(m_state, "CONSTVAL");
		return true;
	}

}

