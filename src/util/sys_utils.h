// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
namespace SysUtils {
#ifdef __linux__
bool transparent_hugepage_is_enabled();
bool adjust_open_files_limit(size_t expected_value);
#endif
int get_cpu_count();
const char* get_os_name();
std::string get_host_name();
}
