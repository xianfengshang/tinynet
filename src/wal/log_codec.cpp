// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "log_codec.h"
#include "base/io_buffer_stream.h"
#include "logging/logging.h"
#include "base/crypto.h"
#include "base/error_code.h"
#include "base/runtime_logger.h"
#include "base/coding.h"

namespace tinynet {
namespace wal {

LogCodec::LogCodec():
    decode_status_(DecodeStatus::Begin),
    decode_len_(0),
    bytes_read_(0),
    bytes_write_(0) {
}

LogCodec::~LogCodec() = default;

void LogCodec::Write(io::FileStreamPtr& stream, const void *data, size_t len) {
    const char* p = static_cast<const char*>(data);
    uint8_t header[LOG_HEADER_SIZE];
    Opcode opcode = Opcode::Binary;
    while (len > 0) {
        size_t block_offset = stream->Length() % LOG_BLOCK_SIZE;
        size_t block_space = LOG_BLOCK_SIZE - block_offset;
        if (block_space < LOG_HEADER_SIZE) {
            if (block_space > 0) {
                bytes_write_ += (int)block_space;
                stream->Write("\x00\x00\x00\x00\x00\x00", block_space);
            }
            block_offset = 0;
            continue;
        }
        block_space -= LOG_HEADER_SIZE;
        uint16_t payloadlen = (uint16_t)(std::min)(len, block_space);
        header[0] = (uint8_t)opcode;
        if (len == payloadlen) {
            header[0] |= 0x80;
        }
        header[1] = (char)(payloadlen & 0xff);
        header[2] = (char)(payloadlen >> 8);
        EncodeFixed32((char*)&header[3], Crypto::crc32(p, payloadlen));
        // write fragment header
        stream->Write(header, LOG_HEADER_SIZE);
        // write payload data
        stream->Write(p, payloadlen);
        p += payloadlen;
        len -= payloadlen;
        opcode = Opcode::Continue;
        bytes_write_ += LOG_HEADER_SIZE;
        bytes_write_ += payloadlen;
    }
}

bool LogCodec::Read(io::FileStreamPtr& stream, std::string* buffer) {
    while (Decode(stream, buffer)) {
    }
    if (decode_status_ == DecodeStatus::Record) {
        decode_status_ = DecodeStatus::End;
        return true;
    }
    return false;
}

bool LogCodec::DecodeHeader() {
    //first byte
    uint8_t byte = iov_.base[0];
    header_.fin = byte >> 7;
    uint8_t opcode = byte & 0x0f;
    if (opcode != static_cast<uint8_t>(Opcode::Continue)) {
        header_.opcode = opcode;
    }
    //second byte
    uint8_t low = iov_.base[1];
    uint8_t high = iov_.base[2];
    header_.len = (high << 8) | low;
    header_.crc = DecodeFixed32(&iov_.base[3]);
    return true;
}

bool LogCodec::Decode(io::FileStreamPtr& stream, std::string* buffer) {
    switch (decode_status_) {
    case DecodeStatus::Begin: {
        iov_.len = 	(unsigned long)stream->Read(buffer_, sizeof(buffer_));
        iov_.base = buffer_;
        if (iov_.len == 0) {
            return false;
        }
        decode_status_ = iov_.len >= LOG_HEADER_SIZE ? DecodeStatus::Header : DecodeStatus::Error;
        return true;
    }
    case DecodeStatus::Header: {
        if (iov_.len < LOG_HEADER_SIZE) {
            decode_len_ += iov_.len;
            decode_status_ = DecodeStatus::Begin;
            return true;
        }
        if (!DecodeHeader()) {
            return false;
        }
        iov_.base += LOG_HEADER_SIZE;
        iov_.len -= LOG_HEADER_SIZE;
        decode_len_ += LOG_HEADER_SIZE;
        decode_status_ = DecodeStatus::Payload;
        return true;
    }
    case DecodeStatus::Payload: {
        if (iov_.len < header_.len) {
            decode_status_ = DecodeStatus::Error;
            return false;
        }
        buffer->append(iov_.base, header_.len);
        iov_.base += header_.len;
        iov_.len -= header_.len;
        decode_len_ += header_.len;
        decode_status_ = header_.fin  ? DecodeStatus::Record : DecodeStatus::Header;
        return true;
    }
    case DecodeStatus::Record: {
        bytes_read_ += decode_len_;
        decode_len_ = 0;
        return false;
    }
    case DecodeStatus::End: {
        decode_status_ = DecodeStatus::Header;
        return true;
    }
    case  DecodeStatus::Error: {
        return false;
    }
    default:
        break;
    }
    return false;
}
}
}
