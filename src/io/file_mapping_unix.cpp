// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include "file_mapping_unix.h"
#include "util/fs_utils.h"
namespace tinynet {
namespace io {
FileMappingUnix::FileMappingUnix(int fd) {
    size_t len = FileSystemUtils::file_size(fd);
    if (len > 0) {
        void* mapping = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
        if (mapping != MAP_FAILED) {
            data_ = (char*)mapping;
            len_ = len;
        }
    }
}

FileMappingUnix::~FileMappingUnix() {
    if (data_) {
        munmap(data_, len_);
        data_ = NULL;
    }
}
}
}
#endif
