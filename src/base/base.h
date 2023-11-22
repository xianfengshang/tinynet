// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <string>
#include <memory>

#ifdef _MSC_VER // for MSVC
#define TINYNET_FORCEINLINE __forceinline
#elif defined __GNUC__ // for gcc on Linux/Apple OS X
#define TINYNET_FORCEINLINE __inline__ __attribute__((always_inline))
#else
#define TINYNET_FORCEINLINE inline
#endif
#if defined(NDEBUG)
# define TINYNET_ASSERT(exp) ((void)0)
#else
# define TINYNET_ASSERT(exp)  assert(exp)
#endif

#define ALIGN_UP(VALUE, ALIGN) (((VALUE) + ((ALIGN) - 1)) & ~((ALIGN) - 1))

#define ALIGN_DOWN(VALUE, ALIGN) ((VALUE) & ~((ALIGN) - 1))


#if defined(__GNUC__) || defined(__clang__)
#define TINYNET_LIKELY(x)     __builtin_expect(!!(x),1)
#define TINYNET_UNLIKELY(x)   __builtin_expect(!!(x),0)
#else
#define TINYNET_LIKELY(x)	(x)
#define TINYNET_UNLIKELY(x)	(x)
#endif

template<typename Callable, typename... Args>
inline void Invoke(Callable& f, Args&&... args) noexcept { if(f)f(std::forward<Args>(args)...); }