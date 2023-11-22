// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "file_stream.h"
#include "util/fs_utils.h"
namespace tinynet {
namespace io {

struct FilePosGuard {
    FilePosGuard(FILE* stream):
        stream_(stream) {
        fgetpos(stream_, &pos_);
    }
    ~FilePosGuard() {
        fsetpos(stream_, &pos_);
    }
    FILE* stream_;
    fpos_t pos_;
};

FileStream::FileStream(FILE* stream):
    stream_(stream) {
    setvbuf(stream, buf_, _IOFBF, sizeof(buf_));
}

FileStream::~FileStream() {
    fclose(stream_);
    stream_ = NULL;
}

FileStreamPtr FileStream::Open(const char* filename, const char* mode) {
    FILE* stream = fopen(filename, mode);
    if (stream == NULL) return FileStreamPtr();
    return std::make_shared<FileStream>(stream);
}

FileStreamPtr FileStream::OpenReadable(const char* filename) {
    return Open(filename, "rb+");
}

FileStreamPtr FileStream::OpenWritable(const char* filename) {
    return Open(filename, "wb");
}

FileStreamPtr FileStream::OpenAppendable(const char* filename) {
    return Open(filename, "ab+");
}

size_t FileStream::Read(void* buf, size_t len) {
    return fread(buf, sizeof(char), len, stream_) * sizeof(char);
}

size_t FileStream::Write(const void *buf, size_t len) {
    return fwrite(buf, sizeof(char), len, stream_) * sizeof(char);
}

int FileStream::Seek(uint32_t offset) {
    return fseek(stream_, offset, SEEK_SET);
}

int FileStream::Flush() {
    return fflush(stream_);
}

size_t FileStream::Length() {
    FilePosGuard guard(stream_);
    fseek(stream_, 0, SEEK_END);
    size_t len = ftell(stream_);
    return len;
}

void FileStream::Truncate(int size) {
    FileSystemUtils::file_truncate(get_fd(), size);
}
}
}