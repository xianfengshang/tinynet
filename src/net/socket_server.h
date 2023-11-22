// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include "listener.h"
#include "socket_channel.h"
#include "ssl_context.h"

namespace tinynet {
namespace net {
/**
 * @brief Socket server options
 *
 */
struct ServerOptions {
    std::string name;
    std::string cert_file;
    std::string key_file;
    std::string listen_path; ///< Unix domain socket
    std::string listen_url;
    std::string listen_ip{"*"};
    int listen_port{ 0 };
    std::tuple<int, int> listen_ports; ///< Use as port range if listen_port == -1
    int keepalive_ms{ 0 };
    bool reuseport{ false };
    bool ipv6only{ false };
    bool debug{ false };
    int max_packet_size{ 0 };
};

typedef std::shared_ptr<ServerOptions> ServerOptionsPtr;

/**
 * @brief Basic socket server
 *
 */
class SocketServer {
  public:
    /**
     * @brief Construct a new Socket Server object
     *
     * @param loop
     */
    SocketServer(EventLoop *loop);
    /**
     * @brief Construct a new Socket Server object
     *
     * @param loop
     * @param ctx
     */
    SocketServer(EventLoop *loop, SSLContext *ctx);
    /**
     * @brief Destroy the Socket Server object
     *
     */
    virtual ~SocketServer();

  public:
    /**
     * @brief General startup function
     *
     * @param opts
     * @return int
     */
    virtual int Start(ServerOptions& opts);
    /**
     * @brief Shutdown the socket server
     *
     */
    virtual void Stop();
    /**
     * @brief Create a Channel object
     *
     * @param sock
     * @return SocketChannelPtr
     */
    virtual SocketChannelPtr CreateChannel(SocketPtr sock) = 0;

  public:
    /**
     * @brief Handle client connection event
     *
     * @param sock
     */
    virtual void HandleAccept(SocketPtr sock);
    /**
     * @brief Handle error event
     *
     * @param err
     */
    virtual void HandleError(int err);

  public:
    /**
     * @brief Remove the socket channel identified by the guid
     *
     * @param guid
     */
    virtual void RemoveChannel(ChannelID guid);
    /**
     * @brief Get the Channel object
     *
     * @param guid
     * @return SocketChannelPtr
     */
    SocketChannelPtr GetChannel(ChannelID guid);

    size_t channel_size() const { return channels_.size(); }

    EventLoop *event_loop() { return event_loop_; }

    template <typename T>
    std::shared_ptr<T> GetChannel(ChannelID guid) {
        return std::static_pointer_cast<T>(GetChannel(guid));
    }
    /**
     * @brief Get the name
     *
     * @return const char*
     */
    const char* get_name() const { return opts_.name.c_str(); }

    int get_listen_port() const { return listener_ ? listener_->get_listen_port() : opts_.listen_port; }

    const net::ServerOptions& get_opts() { return opts_; }
  protected:
    void RemoveChannels();

    void set_listener(net::ListenerPtr listener);

  private:
    ListenerPtr CreateListener();

  protected:
    using CHANNEL_MAP = std::unordered_map<ChannelID, SocketChannelPtr>;
    using CHANNEL_ID_SET = std::unordered_set<ChannelID>;
    CHANNEL_MAP			channels_;
    ListenerPtr			listener_;
    EventLoop *			event_loop_;
    CHANNEL_ID_SET		removing_channels_;
    int64_t				removing_task_;
    SSLContext *		ssl_ctx_;
    ServerOptions		opts_;
};
}
}
