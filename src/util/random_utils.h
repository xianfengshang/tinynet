// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
namespace RandomUtils {
uint8_t Random8();

uint8_t Random8(uint8_t start, uint8_t end);

uint16_t Random16();

uint16_t Random16(uint16_t start, uint16_t end);

uint32_t Random32();

uint32_t Random32(uint32_t start, uint32_t end);

uint64_t Random64();

uint64_t Random64(uint64_t start, uint64_t end);

float RandomFloat();

float RandomFloat(float start, float end);

double RandomDouble();

double RandomDouble(double start, double end);

}
