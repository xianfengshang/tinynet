// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
#include <string>
namespace mysql {
uint8_t charset_to_id(const char* name);

const char* charset_to_name(uint8_t id);
}