#pragma once
#include "UVConfig.h"

BEG_NAME_SPACE

/** 所有不可拷贝的对象都继承于此 */
class Uncopyable {
public:
	Uncopyable() = default;
	Uncopyable(const Uncopyable&) = delete;
	Uncopyable& operator=(const Uncopyable&) = delete;
};

#define ENUM_LOGICAL_OPERATION(ENUM_NAME, ENUM_TYPE) \
inline ENUM_NAME operator&(ENUM_NAME a, ENUM_NAME b) { return ENUM_NAME((ENUM_TYPE)a & (ENUM_TYPE)b); } \
inline ENUM_NAME& operator&=(ENUM_NAME& a, ENUM_NAME b) { return a = a & b; } \
inline ENUM_NAME operator|(ENUM_NAME a, ENUM_NAME b) { return ENUM_NAME((ENUM_TYPE)a | (ENUM_TYPE)b); } \
inline ENUM_NAME& operator|=(ENUM_NAME& a, ENUM_NAME b) { return a = a | b; } \
inline ENUM_NAME operator^(ENUM_NAME& a, ENUM_NAME b) { return ENUM_NAME((ENUM_TYPE)a ^ (ENUM_TYPE)b); } \
inline ENUM_NAME& operator^=(ENUM_NAME& a, ENUM_NAME b) { return a = a ^ b; } \
inline ENUM_NAME operator~(ENUM_NAME a) { return ENUM_NAME(~((ENUM_TYPE)a)); }

END_NAME_SPACE
