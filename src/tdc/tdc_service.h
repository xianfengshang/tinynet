// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "naming/naming_resolver.h"
#include "rpc/rpc_server.h"
#include "net/event_loop.h"
#include "tdc_channel.h"
#include <functional>
#include <tuple>

namespace tinynet {
namespace tdc {
typedef std::function<bool (const std::string &msg_body)> TdcReceiveMessageCallback;

struct TdcOptions {
    std::string name;
    std::vector<std::string> tns_addrs; //Naming server address array
    int registrationInterval{ 0 };
    int expiryTime{ 0 };
    bool debugMode{ false };
    std::string nameSpace;
};

//TinyNet distributed communication service
class TdcService {
    friend class TdcChannel;
  public:
    //Constructor
    TdcService(EventLoop *loop);
    ~TdcService();
  public:
    //Initialize tdc service
    void Init(const TdcOptions& opts);
    //Start tdc service with a given port range
    int Start(const std::tuple<int, int>& ports);
    //Start tdc service with a given address
    int Start(const std::string& addr);
    //Stop tdc service
    void Stop();
  public:
    void SendMsg(const std::string& name, const std::string &body, TdcMessageCallback callback);
    void SendMsg(const std::string& name, const void* body,  size_t len, TdcMessageCallback callback);
    void SendMsg(const std::string& name, TdcMessagePtr msg);
  public:
    const std::string& get_root_dir() { return root_dir_; }

    EventLoop * event_loop() { return event_loop_; }

    naming::NamingResolver* get_resolver() { return resolver_.get(); }

    rpc::RpcServer* get_server() { return server_.get(); }

    bool ExistsChannel(const std::string& name);
  private:
    TdcChannelPtr GetChannel(const std::string& name);
    TdcChannelPtr GetChannel(int64_t guid);
    TdcChannelPtr RemoveChannel(const std::string& name);
    void RegisterService();
    void AfterSend(int64_t channel_guid, int64_t msg_guid);
    void AfterResolved(int64_t channel_guid, const naming::NamingReply& reply);
  public:
    bool ParseMessage(const std::string& msg_body);

    void set_receive_msg_callback(TdcReceiveMessageCallback receive_msg_callback) { receive_msg_cb_ = receive_msg_callback; }
  private:
    using ChannelMap = std::unordered_map<std::string, TdcChannelPtr>;
    using Address = std::pair<std::string, std::string>;
  private:
    TdcOptions			options_;
    std::string			root_dir_;
    Address				address_;
    EventLoop*		    event_loop_;
    std::unique_ptr<naming::NamingResolver> resolver_;
    std::unique_ptr<rpc::RpcServer>	server_;
    ChannelMap			channels_;
    int64_t				register_timer_;
    TdcReceiveMessageCallback	receive_msg_cb_;
    uint64_t			failed_count_;
    uint64_t			success_count_;
};
}
}
