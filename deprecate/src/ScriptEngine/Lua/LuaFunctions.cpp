#include "LuaEngine.h"
#include "LuaFunctions.h"
#include <lua.hpp>
#include <iostream>
#include <functional>
#include <vector>

/** 遍历lua表的辅助函数
 * @param L lua状态控制对象
 * @param tableIndex 当前需要遍历的表在栈中的位置
 * @param func 每遍历到一个key-value对时需要执行的操作
 * @remark func函数执行前的状态为(-2, key), (-1, value); 执行完后也需要保证栈为该状态 */
void TableTraversalHelper(lua_State* L, int tableIndex, const std::function<void(lua_State* L)>& func);

int CreateResource(lua_State* L) {
	/** 需要配合ResourceRawOperation，填充一个资源结构体 */
	TableTraversalHelper(L, 1, [](lua_State* L) {
		lua_pushvalue(L, -2);
		std::cout << "key: " << lua_tostring(L, -1) << "\tvalue: " << lua_tostring(L, -2) << '\n';
		lua_pop(L, 1);
	});
	lua_pop(L, 1);
	return 0;
}

int CreateBlock_U8(lua_State* L) {
	std::vector<uint8_t> buffer(lua_gettop(L));
	while (lua_gettop(L)) {
		if (lua_isnumber(L, -1)) {
			LUA_NUMBER num = lua_tonumber(L, -1);
			LUA_INTEGER integer;
			if (lua_numbertointeger(num, &integer))
				buffer.push_back(static_cast<uint8_t>(integer));
			else {
				luaL_error(L, "NUMBER CANNOT CONVERT TO INTEGER!");
				return 0;
			}
		}
		else {
			luaL_error(L, "CAN NOT USE DATA OTHER THAN NUMBER TO INITIALIZE BLOCK");
			return 0;
		}
		lua_pop(L, 1);
	}
	/** 向栈中压入一个数据块, stack: (-1, userdata)*/
	void* addr = lua_newuserdata(L, buffer.size() * sizeof(uint8_t));
	memcpy(addr, buffer.data(), buffer.size() * sizeof(uint8_t));
	/** 为userdata设置metatable，或者说赋予类型 */
	luaL_newmetatable(L, "BLOCK"); /**< stack: (-1, BLOCK), (-2, userdata) */
	lua_setmetatable(L, -2); /**< stack: (-1, userdata) */
	return 1;
}

void TableTraversalHelper(lua_State* L, int tableIndex, const std::function<void(lua_State* L)>& func) {
	lua_pushnil(L); /** ==> (tableIndex, table), ... , (-1, nil) */
	while (lua_next(L, tableIndex)) { /** 当表没有更多Key后返回0 */
		/** ==> (tableIndex, table), ..., (-2, key), (-1, value) */
		func(L);
		lua_pop(L, 1); /** ==> (-1, key) */
	}
}
