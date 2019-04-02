#ifndef LUA_ENGINE_H
#define LUA_ENGINE_H
#include "../ScirptEngine.h"
struct lua_State;
namespace UnknownVision {
	class LuaEngine : public ScriptEnegine{
	public:
		LuaEngine() = default;
		~LuaEngine();
	public:
		/** 初始化脚本引擎 */
		bool Initialize();
		/** 分析脚本
		 * @param mainFile 包含main函数的脚本文件 */
		void AnalyseScript(const char* script);
	private:
		/** 向脚本引擎注册函数 */
		bool setupFunctions();
		/** 向脚本引擎注册常量 */
		bool setupGlobalVariable();
	private:
		lua_State* m_state = nullptr; /**< lua关键对象 */
	};
}

#endif // LUA_ENGINE_H
