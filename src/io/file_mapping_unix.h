// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifndef _WIN32
#pragma once
#include "file_mapping.h"
namespace tinynet {
namespace io {
class FileMappingUnix :
    public FileMappingImpl {
  public:
    FileMappingUnix(int fd);
    ~FileMappingUnix();
};
}
}
#endif
