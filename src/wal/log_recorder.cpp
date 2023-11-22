// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "log_recorder.h"
namespace tinynet {
namespace wal {
LogRecorder::LogRecorder(io::FileStreamPtr stream) :
    stream_(stream) {
}

void LogRecorder::Put(const std::string& record) {
    codec_.Write(stream_, record.data(), record.size());
}

bool LogRecorder::Next(std::string* record) {
    return codec_.Read(stream_, record);
}

}
}
