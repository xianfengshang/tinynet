// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
namespace tinynet {

enum struct Endian  {
    LittleEndian = 1,
    BigEndian = 2
};
extern Endian BYTE_ORDER_TYPE;

extern bool IsLittleEndian;

void EncodeFixed16(char* buffer, uint16_t value);

void EncodeFixed24(char* buffer, uint32_t value);

void EncodeFixed32(char* buffer, uint32_t value);

void EncodeFixed64(char* buffer, uint64_t value);

uint16_t DecodeFixed16(const char* buffer);

uint32_t DecodeFixed24(const char* buffer);

uint32_t DecodeFixed32(const char* buffer);

uint64_t DecodeFixed64(const char* buffer);
}
