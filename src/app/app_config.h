// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "app_types.h"
#include "base/singleton.h"
#include <vector>
namespace tinynet {
namespace app {
class AppConfig :
    public tinynet::Singleton<AppConfig> {
  public:
    AppConfig();
    ~AppConfig();
  public:
    int Init(const std::string& app_names, const std::string& app_labels);
  private:
    bool ParseLabels(AppMeta& meta, const std::string& app_labels);
  public:
    const std::vector<AppMeta>& get_metas() const { return meta_infos_; }
  private:
    std::vector<AppMeta> meta_infos_;
};
}
}

#define g_AppConfig tinynet::app::AppConfig::Instance()
