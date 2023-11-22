// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "base/module.h"
class AppInit :
    public tinynet::Module<AppInit> {
  public:
    AppInit();
    ~AppInit();
  public:
    int Init();
    int Stop();
    int Priority() const { return 65535; }
  private:
    void ParseCommandLineArgs();
#ifdef __linux__
    void LinuxMemoryWarning();
    void AdjustOpenFilesLimit();
#endif
  private:
    std::string log_dir_;
    std::string app_names_;
    std::string app_labels_;
};
