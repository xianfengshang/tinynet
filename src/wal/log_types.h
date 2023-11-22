// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
#include <stdlib.h>
namespace tinynet {
namespace wal {
enum class Opcode {
    Continue = 0x0,
    Text = 0x1,
    Binary = 0x2
};

/*
WAL fragment header
 0				1				2				3
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+---------+-------------------------------+--------------+
|F|R|R|R|opcode |		payload length			|	crc32	   |
|I|S|S|S|  (4)	|		 (2bytes)				| (1/4byte)	   |
|N|V|V|V|		|								|			   |
| |1|2|3|		|								|			   |
+-+-+-+---------+-------------------------------+--------------+
| crc32 continued (3/4bytes)					| payload data |
+-+-+-+---------+-------------------------------+--------------+
|					payload data continued					   |
+-+-+-+---------+-------------------------------+--------------+
|					payload data continued					   |
+-+-+-+---------+-------------------------------+--------------+
*/

struct LogHeader {
    uint8_t fin{ 0 };  // 1bit
    uint8_t opcode{ 0 }; // 4bit
    uint16_t len{ 0 }; // 2bytes payload data length
    uint32_t crc{ 0 }; // 4bytes crc32
};

constexpr size_t LOG_HEADER_SIZE = 1 + 2 + 4;

constexpr size_t LOG_BLOCK_SIZE = 32768;

}
}
