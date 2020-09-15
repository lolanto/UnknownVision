#pragma once
#include <cstdint>

#define allow_logical_operation
#define PROJECT_NAME_SPACE UnknownVision
#define BEG_NAME_SPACE namespace PROJECT_NAME_SPACE {
#define END_NAME_SPACE }

#define thread_safe
#define thread_safe_const const
#define DX12 1
#define API_TYPE DX12 // 当前使用的API类型

BEG_NAME_SPACE
constexpr uint32_t MAX_RENDER_TARGET = 8; /**< 可绑定的渲染对象的上限 */
constexpr uint32_t MAX_VERTEX_BUFFER = 8; /**< 可绑定的顶点缓冲的上限 */
constexpr uint8_t NUMBER_OF_BACK_BUFFERS = 2; /**< 交换链总共两个后台缓冲 */


END_NAME_SPACE
