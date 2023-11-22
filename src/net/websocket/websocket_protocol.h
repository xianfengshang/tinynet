// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <cstdint>
#include <string>
#include "base/string_view.h"
namespace tinynet {
namespace websocket {

const int MAX_FRAME_HEADER_LENGTH = 14;

enum WebSocketVersion {
    WS_DraftHybi00 = 0,
    WS_DraftHybi10 = 8,
    WS_Rfc6455 = 13
};

enum class Opcode {
    Continue = 0x0,
    Text = 0x1,
    Binary = 0x2,
    Close = 0x8,
    Ping = 0x9,
    Pong = 0xA
};

struct WebSocketPacket {
    uint8_t       fin{0};            // 1bit
    uint8_t       opcode{0};         // 4bit
    uint8_t       mask{0};           // 1bit
    uint8_t       masking_key[4]; // 0 or 4 bytes
    uint64_t      payload_length{0}; // 1 or 2 or 8 bytes
    //std::string   data;
};

struct WebSocketMessage {
    Opcode		opcode{ Opcode::Binary };
    std::string data;
    const char* data_ref{ nullptr };
    size_t		data_len{ 0 };
    std::string* bytes_ref{ nullptr };
};

}
}
