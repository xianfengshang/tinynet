// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "base/io_buffer.h"
#include "base/net_types.h"
#include <functional>
namespace tinynet {
/// Forward declaration of event loop class
class EventLoop;
namespace net {

/// Wrapper class of OS socket file descriptor.
/// On Unix-like system, a socket descriptor is simply an integer associated  with an open socket file, but
/// on Windows, it's defined as SOCKET.
/// Here we abstract the differences by simply using int.
/// It's safe to cast SOCKET to int so far on Windows.
class FileDescriptor:
    public std::enable_shared_from_this<FileDescriptor> {
  public:
    enum FLAGS{
      FD_FLAGS_CLIENT_FD = 0x01,
      FD_FLAGS_SERVER_FD = 0x02,
      FD_FLAGS_LISTEN_FD = 0x04
    };
    typedef std::function<void()> EventCallback;
    typedef std::function<void(int)> ErrorCallback;
  public:
    /// Construct a socket descriptor object with a OS fd
    FileDescriptor(tinynet::EventLoop* loop, int fd);
    virtual ~FileDescriptor();
  public:
#if __cplusplus < 201703L
    std::weak_ptr<FileDescriptor> weak_from_this() noexcept { return std::weak_ptr<FileDescriptor>(shared_from_this()); }
#endif
  public:
    /// FileDescriptor descriptor getter
    int get_fd() const { return fd_; }

    void set_read_callback(EventCallback cb) { read_callback_ = std::move(cb); }

    void set_write_callback(EventCallback cb) { write_callback_ = std::move(cb); }

    void set_error_callback(ErrorCallback cb) { error_callback_ = std::move(cb); }

    void set_close_callback (EventCallback cb) { close_callback_ = std::move(cb); }

    EventLoop* event_loop() { return event_loop_; }
  protected:
    bool has_error() { return mask_ & EVENT_ERROR; }

    bool is_readable() { return mask_ & EVENT_READABLE; }

    bool is_writable() { return mask_ & EVENT_WRITABLE; }
  protected:
    int AddEvent(int mask);

    void ClearEvent(int mask);

    static void handle_event(std::weak_ptr<FileDescriptor> weakfdptr, int fd, int events);

    virtual void HandleEvent(int events);

    virtual void Readable() {}

    virtual void Writable() {}

  private:
    void Dispose(bool disposed) noexcept;
  public:
    ///Close() closes the underlying socket fd and unsubscribes all events from event loop.
    virtual void Close();
    ///SetError() marks the socket descriptor as error and calls the error event handler with the given err.
    virtual void SetError(int err);
  protected:
    EventLoop*			event_loop_;
    int             fd_;
    int					    mask_;
    int             flag_;
    EventCallback		read_callback_;
    EventCallback		write_callback_;
    EventCallback		close_callback_;
    ErrorCallback		error_callback_;
};
}	//namespace net
}	//namespace tinynet
