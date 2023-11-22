// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifdef _WIN32
#pragma once
#include <Windows.h>
#include "file_mapping.h"
namespace tinynet {
namespace io {
class FileMappingWin :
    public FileMappingImpl {
  public:
    FileMappingWin(HANDLE fd);
    ~FileMappingWin();
  private:
    HANDLE mapping_handle_;
};
}
}
#endif