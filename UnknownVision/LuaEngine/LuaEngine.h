#ifndef LUA_ENGINE_H
#define LUA_ENGINE_H

class lua_State;
namespace UnknownVision {
	class LuaEngine {
	public:
		LuaEngine() = default;
		~LuaEngine();
	public:
		/** 初始化脚本引擎 */
		bool Initialize();
		/** 运行脚本
		 * @param mainFile 包含main函数的脚本文件 */
		void Run(const char* mainFile);
	private:
		/** 向脚本引擎注册函数 */
		bool setupFunctions();
	private:
		lua_State* m_state = nullptr; /**< lua关键栈 */
	};
}

#endif // LUA_ENGINE_H
