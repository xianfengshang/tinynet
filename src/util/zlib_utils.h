// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "base/io_buffer.h"
namespace ZlibUtils {
int deflate(unsigned char* data, size_t len, tinynet::IOBuffer* out_buf);

int inflate(unsigned char* data, size_t len, tinynet::IOBuffer* out_buf);
}