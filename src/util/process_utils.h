// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
namespace ProcessUtils {
//Get current process id
int get_pid();
//Get current thread id
int get_tid();

void set_process_title(const char* title);
}
