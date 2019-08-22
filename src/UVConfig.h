#pragma once
#include "Utility/TypeRestriction/TypeRestriction.h"
#include <cstdint>

#define allow_logical_operation
#define PROJECT_NAME_SPACE UnknownVision
#define BEG_NAME_SPACE namespace PROJECT_NAME_SPACE {
#define END_NAME_SPACE }

#define thread_safe
#define thread_safe_const const

#define ENUM_LOGICAL_OPERATION(ENUM_NAME, ENUM_TYPE) \
inline ENUM_NAME operator&(ENUM_NAME a, ENUM_NAME b) { return ENUM_NAME((ENUM_TYPE)a & (ENUM_TYPE)b); } \
inline ENUM_NAME& operator&=(ENUM_NAME& a, ENUM_NAME b) { return a = a & b; } \
inline ENUM_NAME operator|(ENUM_NAME a, ENUM_NAME b) { return ENUM_NAME((ENUM_TYPE)a | (ENUM_TYPE)b); } \
inline ENUM_NAME& operator|=(ENUM_NAME& a, ENUM_NAME b) { return a = a | b; } \
inline ENUM_NAME operator^(ENUM_NAME& a, ENUM_NAME b) { return ENUM_NAME((ENUM_TYPE)a ^ (ENUM_TYPE)b); } \
inline ENUM_NAME& operator^=(ENUM_NAME& a, ENUM_NAME b) { return a = a ^ b; } \
inline ENUM_NAME operator~(ENUM_NAME a) { return ENUM_NAME(~((ENUM_TYPE)a)); }

BEG_NAME_SPACE
constexpr uint32_t MAX_RENDER_TARGET = 8; /**< 可绑定的渲染对象的上限 */
constexpr uint32_t MAX_VERTEX_BUFFER = 8; /**< 可绑定的顶点缓冲的上限 */
constexpr uint8_t NUMBER_OF_BACK_BUFFERS = 2; /**< 交换链总共两个后台缓冲 */

/** 使用constexpr相当于让变量内联 */

constexpr char* VERTEX_BUFFER_NAME[MAX_VERTEX_BUFFER] = {
	"VTXBUF0", "VTXBUF1", "VTXBUF2", "VTXBUF3",
	"VTXBUF4", "VTXBUF5", "VTXBUF6", "VTXBUF7"
};

constexpr char* INDEX_BUFFER_NAME = "IDXBUF";

constexpr char* DEPTH_TARGET_NAME[MAX_RENDER_TARGET] = {
	"DEPTH0", "DEPTH1", "DEPTH2", "DEPTH3",
	"DEPTH4", "DEPTH5", "DEPTH6", "DEPTH7",
};

constexpr char* RENDER_TARGET_NAME[MAX_RENDER_TARGET] = {
	"RTVAL0", "RTVAL1", "RTVAL2", "RTVAL3",
	"RTVAL4", "RTVAL5", "RTVAL6", "RTVAL7"
};

enum COMMAND_UNIT_TYPE {
	DEFAULT_COMMAND_UNIT = 0,
	COMPUTE_COMMAND_UNIT,
	TRANSFER_COMMAND_UNIT,
	NUMBER_OF_COMMAND_UNIT_TYPE
};

END_NAME_SPACE
