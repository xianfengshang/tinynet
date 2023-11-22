// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "naming_state.h"
#include "raft/raft_types.h"
#include "raft/raft_node.h"
#include "raft/raft_service.h"
#include "rpc/zero_copy_stream.h"
#include "logging/logging.h"
#include "naming.pb.h"
#include "base/error_code.h"
#include "rpc/rpc_helper.h"
#include "util/string_utils.h"

#define TRACE_LOG(fmt, ...) node_->Trace(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

namespace tinynet {
namespace naming {

NamingState::NamingState(raft::RaftService* raft_service):
    raft_(raft_service) {
}

int NamingState::Init(const raft::NodeConfig& raft_config) {
    int err = ERROR_OK;
    node_ = raft_->CreateNode(raft_config, this, &err);
    return err;
}

void NamingState::StateChanged(raft::StateType role) {
}

void NamingState::ApplyEntry(uint64_t logIndex, const std::string &data) {
    naming::ClusterMessage msg;
    if (!msg.ParseFromString(data)) {
        log_error("Parse cluster message failed, logIndex[%lld]", logIndex);
        rpc::RpcInfoPtr ctx = PopCall(logIndex);
        if (ctx) {
            SendRedirect(ctx);
        }
        return;
    }
    switch (msg.opcode()) {
    case PUT_DATA: {
        HandleApplyPut(logIndex, msg);
        break;;
    }
    case DEL_DATA: {
        HandleApplyDel(logIndex, msg);
        break;
    }
    default:
        break;
    }
}

void NamingState::HandleApplyPut(uint64_t logIndex, const ::tinynet::naming::ClusterMessage msg) {
    if (!msg.has_put_data()) {
        return;
    }
    auto& put_data = msg.put_data();
    db_.put(put_data.key(), put_data.value(), put_data.expire());

    auto ctx = PopCall(logIndex);
    if (ctx) {
        auto res = ctx->get_response<naming::ClientResponse>()->mutable_put_res();
        res->set_key(put_data.key());
        res->set_value(put_data.value());
        SendResponse(ctx, ERROR_OK);
    }
}

void NamingState::HandleApplyDel(uint64_t logIndex, const ::tinynet::naming::ClusterMessage msg) {
    if (!msg.has_del_data()) {
        return;
    }
    auto& key = msg.del_data().key();
    db_.del(key);
    auto ctx = PopCall(logIndex);
    if (ctx) {
        auto res = ctx->get_response<naming::ClientResponse>()->mutable_del_res();
        res->set_key(key);
        SendResponse(ctx, ERROR_OK);
    }
}

void NamingState::SaveSnapshot(IOBuffer* buffer) {
    std::vector<kv_pair_ptr_t> output;
    db_.snapshot(node_->Time(), &output);

    SnapshotData snapshot;
    for (auto& item: output) {
        auto kv = snapshot.add_data();
        kv->set_key(item->key);
        kv->set_value(item->value);
        kv->set_expire(item->expire_at);
    }

    rpc::ZeroCopyOutputStream zero_copy_stream(buffer);
    if (!snapshot.SerializeToZeroCopyStream(&zero_copy_stream)) {
        log_error("Save snapshot error");
        return;
    }
    zero_copy_stream.Commit();
}

void NamingState::LoadSnapshot(const char* data, size_t len) {
    SnapshotData snapshot;
    if (!snapshot.ParseFromArray(data, static_cast<int>(len))) {
        log_error("Load snapshot error");
    }
    db_.clear();
    for (int i = 0; i < snapshot.data_size(); ++i) {
        const auto& kv = snapshot.data(i);
        db_.put(kv.key(), kv.value(), kv.expire());
    }
}

void NamingState::Get(::google::protobuf::RpcController* controller, const ::tinynet::naming::ClientRequest* request, ::tinynet::naming::ClientResponse* response, ::google::protobuf::Closure* done) {
    rpc::ClosureGuard guard(done);
    auto& key = request->get_req().key();
    TRACE_LOG("\"Get\" \"%s\"", key.c_str());

    response->set_opcode(GET_RES);
    response->set_error_code(ERROR_OK);
    std::string value;
    int64_t expire_at = 0;
    if (!db_.get(key, &value, &expire_at)) {
        response->set_error_code(ERROR_TNS_NAMENOTFOUND);
        return;
    }
    if (expire_at <= node_->Time()) {
        response->set_error_code(ERROR_TNS_NAMEEXPIRED);
        return;
    }
    response->mutable_get_res()->set_key(key);
    response->mutable_get_res()->set_value(value);
}

void NamingState::Put(::google::protobuf::RpcController* controller, const ::tinynet::naming::ClientRequest* request, ::tinynet::naming::ClientResponse* response, ::google::protobuf::Closure* done) {
    auto call = std::make_shared<rpc::RpcInfo>(0, controller, request, response, done);
    auto& put_req = request->put_req();

    TRACE_LOG("\"Put\" \"%s\" \"%s\" \"%d\"", put_req.key().c_str(), put_req.value().c_str(), put_req.ttl());

    response->set_opcode(PUT_RES);
    if (!node_->is_leader()) {
        SendRedirect(call);
        return;
    }

    naming::ClusterMessage cluster_msg;
    cluster_msg.set_opcode(PUT_DATA);
    cluster_msg.mutable_put_data()->set_key(put_req.key());
    cluster_msg.mutable_put_data()->set_value(put_req.value());
    cluster_msg.mutable_put_data()->set_expire(node_->Time() + put_req.ttl());

    auto entry = node_->NextEntry();
    cluster_msg.SerializeToString(&entry->data);
    calls_.emplace(entry->index, call);
    node_->AppendEntry(std::move(entry));
}

void NamingState::Delete(::google::protobuf::RpcController* controller, const ::tinynet::naming::ClientRequest* request, ::tinynet::naming::ClientResponse* response, ::google::protobuf::Closure* done) {
    auto call = std::make_shared<rpc::RpcInfo>(0, controller, request, response, done);
    auto& del_req = request->del_req();

    TRACE_LOG("\"Delete\" \"%s\"", del_req.key().c_str());

    response->set_opcode(DEL_RES);
    if (!node_->is_leader()) {
        SendRedirect(call);
        return;
    }

    naming::ClusterMessage cluster_msg;
    cluster_msg.set_opcode(PUT_DATA);
    cluster_msg.mutable_put_data()->set_key(del_req.key());


    auto entry = node_->NextEntry();
    cluster_msg.SerializeToString(&entry->data);
    calls_.emplace(entry->index, call);
    node_->AppendEntry(std::move(entry));
}

void NamingState::Keys(::google::protobuf::RpcController* controller, const ::tinynet::naming::ClientRequest* request, ::tinynet::naming::ClientResponse* response, ::google::protobuf::Closure* done) {
    rpc::ClosureGuard guard(done);
    auto& key_prefix = request->keys_req().key();
    TRACE_LOG("\"Keys\" \"%s\"", key_prefix.c_str());

    response->set_opcode(KEYS_RES);
    response->set_error_code(ERROR_OK);

    std::vector<std::string> output;
    db_.keys(key_prefix, &output, node_->Time());

    auto key_res = response->mutable_keys_res();
    for (auto key: output) {
        key_res->add_keys(key);
    }
}

void NamingState::SendResponse(rpc::RpcInfoPtr& call, int error_code) {
    TRACE_LOG("Send response");
    call->get_response<naming::ClientResponse>()->set_error_code(error_code);
    call->Run(ERROR_OK);
}

void NamingState::SendRedirect(rpc::RpcInfoPtr& call) {
    auto response = call->get_response<naming::ClientResponse>();
    if (node_->has_leader()) {
        response->set_error_code(ERROR_TNS_SERVICEREDIRECT);
        response->set_redirect(node_->get_leader_address());
        TRACE_LOG("Send redirect to %s", node_->get_leader_address().c_str());
    } else {
        response->set_error_code(ERROR_RAFT_CLUSTERDOWN);
        TRACE_LOG("Send redirect failed, maybe cluster down");
    }
    call->Run(ERROR_OK);
}

rpc::RpcInfoPtr NamingState::PopCall(uint64_t logIndex) {
    auto it = calls_.find(logIndex);
    if (it == calls_.end()) {
        return nullptr;
    }
    auto call = it->second;
    calls_.erase(it);
    return call;
}

}
}
