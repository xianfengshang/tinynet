// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "file_lock.h"
#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif
namespace tinynet {

#ifdef _WIN32
class FileLockWin :
    public FileLockImpl {
  public:
    FileLockWin(HANDLE fd):
        fd_(fd) {
    }
    ~FileLockWin() = default;
  public:
    void lock() {
        memset(&ol_, 0, sizeof(OVERLAPPED));
        LockFileEx(fd_, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ol_);
    }
    void unlock() {
        UnlockFileEx(fd_, 0, MAXDWORD, MAXDWORD, &ol_);
    }
  private:
    HANDLE fd_;
    OVERLAPPED ol_{ 0 };
};
#else
class FileLockUnix :
    public FileLockImpl {
  public:
    FileLockUnix(int fd) :
        fd_(fd) {
    }
    ~FileLockUnix() = default;
  public:
    void lock() {
        lock_.l_type = F_WRLCK;
        fcntl(fd_, F_SETLKW, &lock_);
    }
    void unlock() {
        lock_.l_type = F_UNLCK;
        fcntl(fd_, F_SETLKW, &lock_);
    }
  private:
    int fd_;
    struct flock lock_;
};
#endif

FileLock::FileLock() = default;

void FileLock::init(int fd) {
    FileLockImpl* impl;
#ifdef _WIN32
    impl = new (std::nothrow) FileLockWin((HANDLE)_get_osfhandle(fd));
#else
    impl = new (std::nothrow) FileLockUnix(fd);
#endif
    impl_.reset(impl);
}

FileLock::~FileLock() = default;

void FileLock::lock() {
    if (impl_) impl_->lock();
}

void FileLock::unlock() {
    if(impl_)impl_->unlock();
}


}
