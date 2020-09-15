#pragma once
#include "UVMarco.h"
#include <cstdint>
#include <nmmintrin.h>
#include <filesystem>
#include "../Utility/TypeRestriction/TypeRestriction.h"
BEG_NAME_SPACE

/** 所有不可拷贝的对象都继承于此 */
class Uncopyable {
public:
	Uncopyable() = default;
	Uncopyable(const Uncopyable&) = delete;
	Uncopyable& operator=(const Uncopyable&) = delete;
};

/** 所有单例类继承于此 */
class Standalone {
public:
	virtual ~Standalone() = default;
protected:
	Standalone() = default;
	Standalone(const Standalone&) = delete;
	Standalone& operator=(const Standalone&) = delete;
};

#define ENUM_LOGICAL_OPERATION(ENUM_NAME, ENUM_TYPE) \
inline ENUM_NAME operator&(ENUM_NAME a, ENUM_NAME b) { return ENUM_NAME((ENUM_TYPE)a & (ENUM_TYPE)b); } \
inline ENUM_NAME& operator&=(ENUM_NAME& a, ENUM_NAME b) { return a = a & b; } \
inline ENUM_NAME operator|(ENUM_NAME a, ENUM_NAME b) { return ENUM_NAME((ENUM_TYPE)a | (ENUM_TYPE)b); } \
inline ENUM_NAME& operator|=(ENUM_NAME& a, ENUM_NAME b) { return a = a | b; } \
inline ENUM_NAME operator^(ENUM_NAME& a, ENUM_NAME b) { return ENUM_NAME((ENUM_TYPE)a ^ (ENUM_TYPE)b); } \
inline ENUM_NAME& operator^=(ENUM_NAME& a, ENUM_NAME b) { return a = a ^ b; } \
inline ENUM_NAME operator~(ENUM_NAME a) { return ENUM_NAME(~((ENUM_TYPE)a)); }

template<typename T, size_t num>
constexpr size_t ArraySize(T(&a)[num]) { return num; }

static const std::filesystem::path FileNameConcatenation(const char* root, const char* file = nullptr) {
	static std::filesystem::path P(__FILE__);
	P.assign(root);
	if (file)
		P.replace_filename(file);
	return P;
}

/** 以下代码块来自外部------------------------------------------------------------------ */
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 

// This requires SSE4.2 which is present on Intel Nehalem (Nov. 2008)
// and AMD Bulldozer (Oct. 2011) processors.  I could put a runtime
// check for this, but I'm just going to assume people playing with
// DirectX 12 on Windows 10 have fairly recent machines.
//#ifdef _M_X64
//#define ENABLE_SSE_CRC32 1
//#else
//#define ENABLE_SSE_CRC32 0
//#endif
//
//#if ENABLE_SSE_CRC32
//#pragma intrinsic(_mm_crc32_u32)
//#pragma intrinsic(_mm_crc32_u64)
//#endif


///** 下面所有的计算都隐藏着条件：
// * 1. 指向的数据必须是4字节对齐的*/
//inline size_t HashRange(const uint32_t* const Begin, const uint32_t* const End, size_t Hash)
//{
//#if ENABLE_SSE_CRC32
//	const uint64_t* Iter64 = (const uint64_t*)((size_t(Begin) + 7) & (~7u));
//	const uint64_t* const End64 = (const uint64_t* const)(size_t(End) & (~7u));
//
//	// If not 64-bit aligned, start with a single u32
//	if ((uint32_t*)Iter64 > Begin)
//		Hash = _mm_crc32_u32((uint32_t)Hash, *Begin);
//
//	// Iterate over consecutive u64 values
//	while (Iter64 < End64)
//		Hash = _mm_crc32_u64((uint64_t)Hash, *Iter64++);
//
//	// If there is a 32-bit remainder, accumulate that
//	if ((uint32_t*)Iter64 < End)
//		Hash = _mm_crc32_u32((uint32_t)Hash, *(uint32_t*)Iter64);
//#else
//	// An inexpensive hash for CPUs lacking SSE4.2
//	for (const uint32_t* Iter = Begin; Iter < End; ++Iter)
//		Hash = 16777619U * Hash ^ *Iter;
//#endif
//
//	return Hash;
//}

//template <typename T> inline size_t HashState(const T* StateDesc, size_t Count = 1, size_t Hash = 2166136261U)
//{
//	static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned"); /**< 这里也限制了指针地址 */
//	return HashRange((uint32_t*)StateDesc, (uint32_t*)(StateDesc + Count), Hash);
//}
/** 以上代码块来自外部------------------------------------------------------------------ */

END_NAME_SPACE
