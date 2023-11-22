// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "file_mapping.h"
#ifdef _WIN32
#include <io.h>
#include "file_mapping_win.h"
#else
#include "file_mapping_unix.h"
#endif
namespace tinynet {
namespace io {

FileMapping::FileMapping(int fd) {
    FileMappingImpl* impl;
#ifdef _WIN32
    impl = new(std::nothrow) FileMappingWin((HANDLE)_get_osfhandle(fd));
#else
    impl = new(std::nothrow) FileMappingUnix(fd);
#endif
    impl_.reset(impl);
}

FileMapping::~FileMapping() = default;

size_t FileMapping::length() const {
    return impl_->length();
}

const char* FileMapping::data() const {
    return impl_->data();
}

bool FileMapping::good() const {
    return impl_->good();
}

}
}