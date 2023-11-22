// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma  once
#include <functional>
#include <memory>
#include <tuple>
#include <vector>
#include "event_loop.h"
#include "socket.h"
namespace tinynet {

namespace net {
class Socket;

constexpr int ksomaxconn = 511;

//Listener base for stream server
class Listener:
    public Socket {
  public:
    typedef std::function<void(std::shared_ptr<Socket>)> ConnectionCallback;
  public:
    Listener(EventLoop *loop);
    virtual ~Listener();
  public:
    //TCP server bind and listen
    int BindAndListen(const std::string &ip, int port, int backlog, int flags);
    //Unix domain socket server bind and listen
    int BindAndListen(const std::string &unix_path, int backlog);
  public:
    const std::string& get_listen_address() const { return listen_address_; }

    int get_listen_port() const { return listen_port_; }

    void set_conn_callback(ConnectionCallback cb) { conn_callback_ = std::move(cb); }
  protected:
    std::string		   listen_address_;
    int				   listen_port_;
    int				   af_;	//address family
    ConnectionCallback conn_callback_;
};

typedef std::shared_ptr<Listener> ListenerPtr;
}
}
