// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <functional>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include "net/event_loop.h"
#include "rpc/rpc_channel.h"
#include "rpc/rpc_controller.h"
#include "naming.pb.h"

namespace tinynet {
namespace naming {
enum class NamingReplyType {
    GET,
    PUT,
    DEL,
    KEYS
};
struct NamingReply {
    NamingReplyType type{ NamingReplyType::GET};
    int err{ 0 };
    std::string value;
    std::vector<std::string> keys;
};

//TinyNet naming service client
class NamingResolver {
  public:
    NamingResolver(EventLoop *loop);
    ~NamingResolver();
  public:
    void Init(const std::vector<std::string> &addrs);
    void Stop();
  private:
    using StubPtr = std::shared_ptr<NamingRpcService_Stub>;

    StubPtr GetStub(const std::string &addr);
    StubPtr GetStub(size_t addr_hint);
    void CacheAddr(const std::string& addr);
  public:
    typedef std::function<void(const NamingReply& reply)> NamingCallback;

    int Put(const std::string &name,
            const std::string &value,
            uint32_t timeout,
            NamingCallback callback);

    int Get(const std::string &name, NamingCallback callback);

    int Delete(const std::string &name, NamingCallback callback);

    int Keys(const std::string& name, NamingCallback callback);
  private:
    struct TnsContext {
        naming::ClientRequest request;
        naming::ClientResponse response;
        rpc::RpcController controller;
        size_t retryCount{ 0 };
        size_t redirectCount{ 0 };
        NamingCallback callback;
    };
    using TnsContextPtr = std::shared_ptr<TnsContext>;
    rpc::RpcChannelPtr CreateChannel(const std::string& ip, int port);

    int Invoke(TnsContextPtr ctx);
    void HandleInvoke(TnsContextPtr ctx);
  private:
    EventLoop * event_loop_;
    std::vector<std::string> addrs_;
    std::string cached_addr_;
    std::vector<rpc::RpcChannelPtr> channels_;
    std::unordered_map<std::string, StubPtr> stubs_;
};
}
}
