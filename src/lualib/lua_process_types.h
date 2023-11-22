// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
namespace tinynet {
namespace lua {
struct ProcessEvent {
    std::string type;
    std::string err;
    int64_t exit_status{ 0 };
    int term_signal{ 0 };
};

struct ProcessOptions {
    std::string file;
    std::vector<std::string> args;
    std::vector<std::string> env;
    std::string cwd;
    unsigned int flags;
    //std::vector<int> stdio;
    //unsigned int uid{ 0 };
    //unsigned int gid{ 0 };
};

}
}
