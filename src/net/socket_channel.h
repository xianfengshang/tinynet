// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <cstdint>
#include <memory>
#include "socket.h"
#include "net/event_loop.h"
namespace tinynet {
namespace net {
typedef int64_t ChannelID;

constexpr ChannelID INVALID_CHANNEL_ID = 0;

class SocketChannel;
typedef std::shared_ptr<SocketChannel> SocketChannelPtr;

class SocketServer;

enum class ChannelState {
    CS_UNSPEC = 0,
    CS_CONNECTING = 1,
    CS_HANDSHAKING = 2,
    CS_CONNECTED = 3,
    CS_CLOSED = 4,
};

/**
 * @brief Options for socket channel
 *
 */
struct ChannelOptions {
    std::string name;
    std::string url;
    std::string host;
    int port{ 0 };
    std::string path;
    int timeout{ 0 };
    int keepalive_ms{ 0 }; ///< keep alive timeout
    int ping_interval{ 0 };///< ping interval
    bool use_ssl{ false }; ///< using secure connection
    std::string ssl_key;
    std::string ssl_cert;
    std::string ssl_ca;
    std::string ssl_capath;
    bool debug{ false };
};

typedef std::shared_ptr<ChannelOptions> ChannelOptionsPtr;

/**
 * @brief Socket channel
 * Basic socket channel represent the socket client/server session
 */
class SocketChannel :
    public std::enable_shared_from_this<SocketChannel> {
    friend class SocketServer;
  public:
    /**
     * @brief Construct a new Socket Channel object
     *
     * @param sock
     * @param server
     */
    SocketChannel(SocketPtr sock, SocketServer* server);
    /**
     * @brief Construct a new Socket Channel object
     *
     * @param loop
     */
    SocketChannel(EventLoop *loop);
    /**
     * @brief Destroy the Socket Channel object
     *
     */
    virtual ~SocketChannel();
  private:
    SocketChannel(const SocketChannel &o) = delete;
    SocketChannel& operator = (const SocketChannel &o) = delete;
  public:
    virtual void Init(ChannelOptions &opts);
    int Open();
    void Close(int err);
  protected:
    void HandleConnect();
    void HandleError(int err);
  protected:
    virtual void OnOpen() {}
    virtual void OnClose() {}
    virtual void OnRead() {}
    virtual void OnError(int err) {}
  protected:
    void set_socket(SocketPtr sock);
  public:
    ChannelID get_guid() const { return guid_; }
    SocketPtr get_socket() { return socket_;}
    template <typename T>
    std::shared_ptr<T> get_socket() { return std::static_pointer_cast<T>(socket_); }
    SocketServer * get_server() { return server_; }
    template<typename T>
    T* get_server() { return static_cast<T*>(server_); }
    ChannelState get_state() const { return state_; }
    const std::string& get_address() const { return address_; }
    const char* get_name() const { return opts_.name.c_str(); }

    void set_keepalive_ms(int value) { opts_.keepalive_ms = value; }

    int64_t get_keepalive_ms() const { return opts_.keepalive_ms; }

    void set_ping_interval(int value) { opts_.ping_interval = value; }

    int get_ping_interval() const { return opts_.ping_interval; }
  protected:
    using ChannelOptionsPtr = std::shared_ptr<ChannelOptions>;
  protected:
    EventLoop *       event_loop_;
    ChannelID		  guid_;        //Channel id
    SocketPtr		  socket_;      //Underlying socket
    SocketServer *	  server_;      //Socket server
    ChannelState      state_;       //Socket channel state
    std::string       address_;     //Peer address
    ChannelOptions    opts_;        //The channel options
    std::unique_ptr<SSLContext>      ssl_ctx_;
};
}
}
