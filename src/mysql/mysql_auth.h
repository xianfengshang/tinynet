// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
#include <string>
namespace mysql {

struct InputAuthPluginData {
    const std::string* password;
    const std::string* auth_plugin_name;
    const std::string* auth_plugin_data;
    const std::string* auth_extra_data;
};

struct OutpuAuthPluginData {
    std::string* auth_response;
    uint32_t* flags;
};

bool invoke_mysql_auth_plugin(const InputAuthPluginData& input, OutpuAuthPluginData& output);
}