// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "file_descriptor.h"
#include "base/net_types.h"
#include "util/net_utils.h"
#include "event_loop.h"
#include "base/error_code.h"
#include "base/io_buffer_stream.h"

namespace tinynet {
namespace net {

FileDescriptor::FileDescriptor(tinynet::EventLoop* loop, int fd):
    event_loop_(loop),
    fd_(fd),
    mask_(0),
    flag_(0) {
}


FileDescriptor::~FileDescriptor() {
    Dispose(true);
}

int FileDescriptor::AddEvent(int mask) {
    return event_loop_->AddEvent(fd_, mask, std::bind(handle_event, weak_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void FileDescriptor::ClearEvent(int mask) {
    return event_loop_->ClearEvent(fd_, mask);
}

void FileDescriptor::handle_event(std::weak_ptr<FileDescriptor> weakfdptr, int fd, int events) {
    if (weakfdptr.expired()) {
        return;
    }
    if (std::shared_ptr<FileDescriptor> fdptr = weakfdptr.lock()) {
        fdptr->HandleEvent(events);
    }

}

void FileDescriptor::HandleEvent(int events) {
    if (events & EVENT_ERROR) {
        SetError(ERROR_SOCKET_CLOSEDBYPEER);
        return;
    }
    if (events & EVENT_READABLE) {
        mask_ |= EVENT_READABLE;
        Readable();
    }
    if (events & EVENT_WRITABLE) {
        mask_ |= EVENT_WRITABLE;
        Writable();
    }
}

void FileDescriptor::Close() {
    Dispose(false);
}

void FileDescriptor::SetError(int err) {
    mask_ |= EVENT_ERROR;
    Invoke(error_callback_,err);
}

void FileDescriptor::Dispose(bool disposed) noexcept {
    if (fd_ == -1) return;
    event_loop_->ClearEvent(fd_, EVENT_FULL_MASK);
    NetUtils::Close(fd_);
    mask_ = EVENT_NONE;

    if (disposed) {
        read_callback_ = nullptr;
        write_callback_ = nullptr;
        error_callback_ = nullptr;
        close_callback_ = nullptr;
    } else {
        Invoke(close_callback_);
    }
}
}
}
