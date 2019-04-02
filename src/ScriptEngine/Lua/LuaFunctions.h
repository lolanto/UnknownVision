#ifndef LUA_FUNCTIONS_H
#define LUA_FUNCTIONS_H
/** 该头文件记录了脚本引擎应该实现的Lua方法 */

struct lua_State;

/** 该方法用于创建引擎资源，包括缓冲区，纹理等
 * @param table 该方法接受一个表，表中存储了资源创建所需要的字段 */
int CreateResource(lua_State* L);

/** 该方法接受若干个uint8作为字节，创建一个字节块
 * @param uint8s 若干个uint8的数字 */
int CreateBlock_U8(lua_State* L);

#endif // LUA_FUNCTIONS_H
