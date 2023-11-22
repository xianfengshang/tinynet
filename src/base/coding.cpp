// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "coding.h"
#include <string.h>
namespace tinynet {
uint16_t BYTE_ORDER_TEST = 0x0201;
Endian BYTE_ORDER_TYPE = (Endian)*reinterpret_cast<uint8_t*>(&BYTE_ORDER_TEST);
bool IsLittleEndian = BYTE_ORDER_TYPE == Endian::LittleEndian;

void EncodeFixed16(char* buffer, uint16_t value) {
    if (IsLittleEndian) {
        memcpy(buffer, &value, sizeof(value));
    } else {
        buffer[0] = value & 0xff;
        buffer[1] = (value >> 8) & 0xff;
    }
}

void EncodeFixed24(char* buffer, uint32_t value) {
    if (IsLittleEndian) {
        memcpy(buffer, &value, 3);
    } else {
        buffer[0] = value & 0xff;
        buffer[1] = (value >> 8) & 0xff;
        buffer[2] = (value >> 16) & 0xff;
    }
}

void EncodeFixed32(char* buffer, uint32_t value) {
    if (IsLittleEndian) {
        memcpy(buffer, &value, sizeof(value));
    } else {
        buffer[0] = value & 0xff;
        buffer[1] = (value >> 8) & 0xff;
        buffer[2] = (value >> 16) & 0xff;
        buffer[3] = (value >> 24) & 0xff;
    }
}

void EncodeFixed64(char* buffer, uint64_t value) {
    if (IsLittleEndian) {
        memcpy(buffer, &value, sizeof(value));
    } else {
        buffer[0] = value & 0xff;
        buffer[1] = (value >> 8) & 0xff;
        buffer[2] = (value >> 16) & 0xff;
        buffer[3] = (value >> 24) & 0xff;
        buffer[4] = (value >> 32) & 0xff;
        buffer[5] = (value >> 40) & 0xff;
        buffer[6] = (value >> 48) & 0xff;
        buffer[7] = (value >> 56) & 0xff;
    }
}

uint16_t DecodeFixed16(const char* buffer) {
    uint16_t value = 0;
    if (IsLittleEndian) {
        memcpy(&value, buffer, sizeof(value));
    } else {
        value = (uint16_t)buffer[0];
        value |= (uint16_t)buffer[1] << 8;
    }
    return value;
}

uint32_t DecodeFixed24(const char* buffer) {
    uint32_t value = 0;
    if (IsLittleEndian) {
        memcpy(&value, buffer, 3);
    } else {
        value = (uint32_t)buffer[0];
        value |= (uint32_t)buffer[1] << 8;
        value |= (uint32_t)buffer[2] << 16;
    }
    return value;
}

uint32_t DecodeFixed32(const char* buffer) {
    uint32_t value = 0;
    if (IsLittleEndian) {
        memcpy(&value, buffer, sizeof(value));
    } else {
        value = (uint32_t)buffer[0];
        value |= (uint32_t)buffer[1] << 8;
        value |= (uint32_t)buffer[2] << 16;
        value |= (uint32_t)buffer[3] << 24;
    }
    return value;
}

uint64_t DecodeFixed64(const char* buffer) {
    uint64_t value = 0;
    if (IsLittleEndian) {
        memcpy(&value, buffer, sizeof(value));
    } else {
        value = (uint64_t)buffer[0];
        value |= (uint64_t)buffer[1] << 8;
        value |= (uint64_t)buffer[2] << 16;
        value |= (uint64_t)buffer[3] << 24;
        value |= (uint64_t)buffer[4] << 32;
        value |= (uint64_t)buffer[5] << 40;
        value |= (uint64_t)buffer[6] << 48;
        value |= (uint64_t)buffer[7] << 56;
    }
    return value;
}
}
