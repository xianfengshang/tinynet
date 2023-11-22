// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "libzip/zip.h"
namespace ZipUtils {

struct zip_error_guard_t {
    zip_error_guard_t(int err) {
        zip_error_init_with_code(&error_, err);
    }

    ~zip_error_guard_t() { zip_error_fini(&error_); }

    const char* what() { return zip_error_strerror(&error_); }

    zip_error_t error_;
};
}