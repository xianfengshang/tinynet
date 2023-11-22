// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
namespace tinynet {
class FileLockImpl {
  public:
    FileLockImpl() {}
    virtual ~FileLockImpl() {}
  public:
    virtual void lock() = 0;
    virtual void unlock() = 0;
};

class FileLock {
  public:
    FileLock();
    ~FileLock();
  public:
    void init(int fd);
  public:
    void lock();
    void unlock();
  private:
    std::unique_ptr<FileLockImpl> impl_;
};
}