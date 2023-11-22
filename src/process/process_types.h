// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <map>
#include <vector>
namespace tinynet {
namespace process {

//Process creation flags, windows only
#define PROCESS_FLAGS_NEW_CONSOLE 1
#define PROCESS_FLAGS_NO_WINDOW 2
#define PROCESS_FLAGS_DETACHED_PROCESS 4
#define PROCESS_FLAGS_UNICODE_ENVIRONMENT 8

struct ProcessOptions {
    std::string file;
    std::vector<std::string> args;
    std::map<std::string, std::string> env;
    std::string cwd;
    int flags{ 0 };
};
}
}
