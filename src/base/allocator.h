// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#ifdef USE_JEMALLOC
#define JEMALLOC_MANGLE
#include "jemalloc/jemalloc.h"
#endif
#ifdef USE_MIMALLOC
#include "mimalloc.h"
#include "mimalloc-override.h"
#endif