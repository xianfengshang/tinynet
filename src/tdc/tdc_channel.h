// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "rpc/rpc_channel.h"
#include "tdc_message.h"
#include "tdc_message_queue.h"
#include "naming/naming_resolver.h"

namespace tinynet {
namespace tdc {
class TdcService;
class TdcChannel;
typedef std::shared_ptr<TdcChannel> TdcChannelPtr;

// TinyNet distributed communication channel
class TdcChannel {
    friend class TdcService;
  public:
    TdcChannel(const std::string& name, TdcService* service);
    ~TdcChannel();
  public:
    enum ChannelState {
        CS_INIT = 0,
        CS_RESOLVING = 1,
        CS_RESOLVED = 2
    };
  public:
    void Init();

    void SendMsg(const std::string &body, TdcMessageCallback callback);
    void SendMsg(const void* body, size_t len, TdcMessageCallback callback);

    void SendMsg(TdcMessagePtr msg);

    void Run(int err);
  public:
    int64_t get_guid() const { return guid_; }

    const std::string& get_name() const { return name_; }
  private:
    void HandleError(int err);

    void Update();

    void UpdateLater();

    void Send();

    void Resend(int err);

    void AfterSend(int64_t msg_guid);

    void Resolve();

    void AfterResolved(const naming::NamingReply& reply);
  private:
    using StubPtr = std::shared_ptr<TdcRpcService_Stub>;
  private:
    int64_t				guid_;
    std::string			name_;
    rpc::RpcChannelPtr  channel_;
    StubPtr				stub_;
    TdcService *		service_;
    ChannelState		state_;
    TdcMessageQueue		send_queue_;
    uint32_t			send_window_;
    std::string			host_;
    int					port_;
    int					retry_count;
    int					error_code_;
    int64_t				timer_guid_;
};
}
}
