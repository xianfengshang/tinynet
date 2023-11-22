// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "base/io_buffer.h"
#include "base/net_types.h"
#include "file_descriptor.h"
#include <functional>
#include <memory>

namespace tinynet {
/// Forward declaration of event loop class
class EventLoop;
namespace net {

/**
 * @brief Socket status
 *
 */
enum class SocketStatus {
    SS_UNSPEC = 0, ///< Unspecified
    SS_CONNECTING = 1,
    SS_CONNECTED = 2,
    SS_CLOSED = 3,
    SS_MAX = 4
};

/**
 * @brief Basic socket class
 *
 */
class Socket:
    public FileDescriptor {
  public:
    /**
    * @brief Construct a new Socket object with a OS fd
    *
    * @param loop
    */
    Socket(EventLoop* loop);
    /**
     * @brief Construct a new Socket object with a OS fd
     *
     * @param loop
     * @param fd
     */
    Socket(EventLoop* loop, int fd, int af, const std::string* peer_address);
    /**
     * @brief Destroy the Socket object
     *
     */
    virtual ~Socket();
  public:
    /**
     * @brief  Access the read buffer
     *
     * @return IOBuffer*
     */
    IOBuffer* rbuf() { return &rbuf_; }
    /**
     * @brief Access the write buffer
     *
     * @return IOBuffer*
     */
    IOBuffer* wbuf() { return &wbuf_; }
  public:
    /**
     * @brief Read() attempts to read up to len bytes from the read buffer into the buffer starting at buf.
     *
     * @param buf
     * @param len
     * @return size_t
     */
    size_t Read(void *buf, size_t len);
    /**
     * @brief Write() puts len bytes from the buffer starting at buf to the write buffer,
     * and attempts to send some data if the underlying socket fd is writable.
     * @param buf
     * @param len
     */
    void Write(const void *data, size_t len);
    /**
     * @brief Flush() attempts to send some data if the underlying socket fd is writable,
     * otherwise subscribes a writable event from event loop.
     */
    void Flush();
    /**
     * @brief Close() closes the underlying socket fd and unsubscribes all events from event loop.
     *
     */
    void Close() override;
    /**
     * @brief Set the Error object
     *
     * @param err
     */
    void SetError(int err) override;
    /**
     * @brief Connect to tcp server using the given host and port
     *
     * @param host
     * @param port
     * @return int
     */
    int Connect(const std::string &host, int port, int timeout = 0);
    /**
     * @brief Connect to Unix domain socket server using the given path
     *
     * @param unix_path
     * @return int
     */
    int Connect(const std::string& unix_path);
  public:
    /**
     * @brief Get the status object
     *
     * @return SocketStatus
     */
    SocketStatus get_status() { return status_; }
    /**
     * @brief Set the status object
     *
     * @param status
     */
    void set_status(SocketStatus status) { status_ = status; }
    /**
     * @brief Return whether the socket is closed or not
     *
     * @return true
     * @return false
     */
    bool is_closed() { return status_ == SocketStatus::SS_CLOSED; }
    /**
     * @brief Return whether the socket is connected or not
     *
     * @return true
     * @return false
     */
    bool is_connected() { return status_ == SocketStatus::SS_CONNECTED; }

    /**
     * @brief Return whether the socket is in connecting status or not
     *
     * @return true
     * @return false
     */
    bool is_connecting() { return status_ == SocketStatus::SS_CONNECTING; }

    /**
     * @brief Set the conn callback object
     *
     * @param cb
     */
    void set_conn_callback(EventCallback cb) { conn_callback_ = std::move(cb); }

    /**
     * @brief Get the peer address object
     *
     * @return const std::string&
     */
    const std::string& get_peer_address() const { return peer_address_; }
  protected:
    /**
     * @brief Open() will be called after the connection made
     *
     */
    virtual void Open();
    /**
     * @brief Read function for raw stream socket
     *
     */
    void StreamReadable();
    /**
     * @brief Write function for raw stream socket
     *
     */
    void StreamWritable();
  private:
    void Dispose(bool disposed) noexcept;
  protected:
    int			  af_; ///< Adress family
    SocketStatus  status_; ///< Socket status
    IOBuffer      rbuf_; ///< Read buffer
    IOBuffer      wbuf_; ///< Write buffer
    EventCallback conn_callback_; ///< Connection callback
    std::string   peer_address_; ///< Peer address
    int64_t       connect_timer_;
};
typedef std::shared_ptr<Socket> SocketPtr;

}	//namespace net
}	//namespace tinynet
