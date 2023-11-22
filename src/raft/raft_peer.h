// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "rpc/rpc_channel.h"
#include "rpc/rpc_controller.h"
#include "net/event_loop.h"
#include "raft.pb.h"
namespace tinynet {
namespace raft {
class RaftService;
class RaftPeer;
typedef std::shared_ptr<RaftPeer> RaftPeerPtr;

class RaftPeer :
    public std::enable_shared_from_this<RaftPeer> {
    friend class RaftService;
  public:
    RaftPeer(EventLoop *loop, int peerId);

    ~RaftPeer();
  public:

    void Init(const std::string& url);
    void Close();
  public:

    typedef std::function<void(int err, const VoteResp *)> RequestVoteCallback;

    void RequestVote(const VoteReq& req, RequestVoteCallback callback);

    typedef std::function<void(int err, const AppendEntriesResp *)> AppendEntriesCallback;

    void AppendEntries(const AppendEntriesReq& req, AppendEntriesCallback callback);

    typedef std::function<void(int err, const InstallSnapshotResp *)> InstallSnapshotCallback;
    void InstallSnapshot(const InstallSnapshotReq& req, InstallSnapshotCallback callback);
  public:

    int get_id() const { return id_; }

    void Run(int err);
  private:
    void HandleError(int err);
  private:
    struct RequestVoteCall {
        VoteReq request;
        VoteResp response;
        rpc::RpcController controller;
    };
    void OnRequestVoteResp(std::shared_ptr<RequestVoteCall> call,
                           RequestVoteCallback callback);

    struct AppendEntriesCall {
        AppendEntriesReq request;
        AppendEntriesResp response;
        rpc::RpcController controller;
    };

    void OnAppendEntriesResp(std::shared_ptr<AppendEntriesCall> call,
                             AppendEntriesCallback callback);
    struct InstallSnapshotCall {
        InstallSnapshotReq request;
        InstallSnapshotResp response;
        rpc::RpcController controller;
    };
    void OnInstallSnapshotResp(std::shared_ptr<InstallSnapshotCall> call,
                               InstallSnapshotCallback callback);
  private:
    using StubPtr = std::unique_ptr<RaftRpcService_Stub>;
    using ChannelPtr = std::unique_ptr<rpc::RpcChannel>;
  private:
    int					id_;
    EventLoop*			event_loop_;
    ChannelPtr			channel_;
    StubPtr				stub_;
    std::string			host_;
    int					port_;
};
}
}
