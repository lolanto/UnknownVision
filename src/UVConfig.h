#pragma once
#include "Utility/TypeRestriction/TypeRestriction.h"
#include <cstdint>

#define allow_logical_operation
#define PROJECT_NAME_SPACE UnknownVision
#define BEG_NAME_SPACE namespace PROJECT_NAME_SPACE {
#define END_NAME_SPACE }

#define thread_safe
#define thread_safe_const const

BEG_NAME_SPACE
const uint32_t UV_MAX_RENDER_TARGET = 8; /**< 可绑定的渲染对象的上限 */
const uint32_t UV_MAX_VERTEX_BUFFER = 16; /**< 可绑定的顶点缓冲的上限 */
END_NAME_SPACE
