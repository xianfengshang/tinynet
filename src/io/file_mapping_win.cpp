// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifdef _WIN32
#include "file_mapping_win.h"
namespace tinynet {
namespace io {
FileMappingWin::FileMappingWin(HANDLE fd):
    FileMappingImpl() {
    mapping_handle_ = CreateFileMapping(fd, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapping_handle_ != NULL) {
        data_ = (char*)MapViewOfFile(mapping_handle_, FILE_MAP_READ, 0, 0, 0);
        len_ = data_ == NULL ? 0 : GetFileSize(fd, NULL);
    }
}

FileMappingWin::~FileMappingWin() {
    if (mapping_handle_) {
        if (data_) {
            UnmapViewOfFile(data_);
            data_ = NULL;
        }
        CloseHandle(mapping_handle_);
        mapping_handle_ = NULL;
    }
}
}
}
#endif
