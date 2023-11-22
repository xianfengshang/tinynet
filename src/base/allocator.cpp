// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "allocator.h"

#ifdef USE_JEMALLOC

#ifdef _WIN32
#include <new>

void operator delete(void* p) noexcept { je_free(p); };
void operator delete[](void* p) noexcept { je_free(p); };

void* operator new(std::size_t n) noexcept(false) { return je_malloc(n); }
void* operator new[](std::size_t n) noexcept(false) { return je_malloc(n); }

void* operator new  (std::size_t n, const std::nothrow_t& tag) noexcept { (void)(tag); return je_malloc(n); }
void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept { (void)(tag); return je_malloc(n); }

#if (__cplusplus >= 201402L || _MSC_VER >= 1916)
void operator delete  (void* p, std::size_t n) noexcept { (void)n; je_free(p); };
void operator delete[](void* p, std::size_t n) noexcept { (void)n; je_free(p);  };
#endif

#if (__cplusplus > 201402L || defined(__cpp_aligned_new))
void operator delete  (void* p, std::align_val_t al) noexcept { (void)al; je_free(p); }
void operator delete[](void* p, std::align_val_t al) noexcept { (void)al; je_free(p); }
void operator delete  (void* p, std::size_t n, std::align_val_t al) noexcept { (void)al; je_free(p); };
void operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept { (void)al; je_free(p); };

void* operator new(std::size_t n, std::align_val_t al)   noexcept(false) { return je_aligned_alloc(n, static_cast<size_t>(al)); }
void* operator new[](std::size_t n, std::align_val_t al) noexcept(false) { return je_aligned_alloc(n, static_cast<size_t>(al)); }
void* operator new  (std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { return je_aligned_alloc(n, static_cast<size_t>(al)); }
void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { return je_aligned_alloc(n, static_cast<size_t>(al)); }
#endif

#else
JEMALLOC_EXPORT void (*__free_hook)(void *ptr) = je_free;
JEMALLOC_EXPORT void *(*__malloc_hook)(size_t size) = je_malloc;
JEMALLOC_EXPORT void *(*__realloc_hook)(void *ptr, size_t size) = je_realloc;
JEMALLOC_EXPORT void *(*__memalign_hook)(size_t alignment, size_t size) = je_memalign;
#endif

#endif

#ifdef USE_MIMALLOC
#include "mimalloc-new-delete.h"
#endif