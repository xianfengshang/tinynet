// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "raft_peer.h"
#include "raft_service.h"
#include "net/stream_socket.h"
#include "base/error_code.h"
#include "logging/logging.h"
#include "util/string_utils.h"
#include "google/protobuf/stubs/common.h"
#include "util/net_utils.h"
#include "util/string_utils.h"
#include "util/uri_utils.h"

namespace tinynet {
namespace raft {

RaftPeer::RaftPeer(EventLoop *loop, int peerId) :
    id_(peerId),
    event_loop_(loop),
    port_(0) {
}

RaftPeer::~RaftPeer() = default;

void RaftPeer::Init(const std::string& url) {
    if (!UriUtils::parse_address(url, &host_, &port_)) {
        log_fatal("Invalid address %s", url.c_str());
        return;
    }
    channel_.reset(new (std::nothrow) rpc::RpcChannel(event_loop_));
    if (!channel_) {
        log_fatal("Create channel failed, out of memory");
        return;
    }
    channel_->Init(host_, port_);

    stub_.reset(new (std::nothrow) RaftRpcService_Stub(channel_.get()));
    if (!stub_) {
        log_fatal("Create RPC stub failed, out of memory");
        return;
    }
}

void RaftPeer::Close() {
    Run(ERROR_RPC_REQUESTCANCELED);
    stub_.reset();
    channel_.reset();
}

void RaftPeer::HandleError(int err) {
    channel_->Reset();
}

void RaftPeer::RequestVote(const VoteReq& req, RequestVoteCallback callback) {
    auto call = std::make_shared<RequestVoteCall>();
    call->request.CopyFrom(req);
    stub_->RequestVote(&call->controller, &call->request, &call->response,
                       ::google::protobuf::NewCallback(this, &RaftPeer::OnRequestVoteResp, call, callback));
}

void RaftPeer::AppendEntries(const AppendEntriesReq& req, AppendEntriesCallback callback) {
    auto call = std::make_shared<AppendEntriesCall>();
    call->request.CopyFrom(req);
    stub_->AppendEntries(&call->controller, &call->request, &call->response,
                         ::google::protobuf::NewCallback(this, &RaftPeer::OnAppendEntriesResp, call, callback));
}

void RaftPeer::InstallSnapshot(const InstallSnapshotReq& req, InstallSnapshotCallback callback) {
    auto call = std::make_shared<InstallSnapshotCall>();
    call->request.CopyFrom(req);
    stub_->InstallSnapshot(&call->controller, &call->request, &call->response,
                           ::google::protobuf::NewCallback(this, &RaftPeer::OnInstallSnapshotResp, call, callback));
}

void RaftPeer::OnRequestVoteResp(std::shared_ptr<RequestVoteCall> call, RequestVoteCallback callback) {
    callback(call->controller.ErrorCode(), &call->response);
}

void RaftPeer::OnAppendEntriesResp(std::shared_ptr<AppendEntriesCall> call, AppendEntriesCallback callback ) {
    callback(call->controller.ErrorCode(), &call->response);
}

void RaftPeer::OnInstallSnapshotResp(std::shared_ptr<InstallSnapshotCall> call, InstallSnapshotCallback callback) {
    callback(call->controller.ErrorCode(), &call->response);
}

void RaftPeer::Run(int err) {
    channel_->Run(err);
}
}
}
