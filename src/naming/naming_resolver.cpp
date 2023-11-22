// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "naming_resolver.h"
#include "util/string_utils.h"
#include "util/uri_utils.h"
#include "base/unique_id.h"
#include "net/stream_socket.h"
#include "logging/logging.h"
#include "base/error_code.h"
#include <functional>
#include "google/protobuf/stubs/common.h"

namespace tinynet {
namespace naming {

const int kMaxRetryCount = 3;

const int kMaxRedirectCount = 3;

NamingResolver::NamingResolver(EventLoop *loop) :
    event_loop_(loop) {
}

NamingResolver::~NamingResolver() = default;

void NamingResolver::Init(const std::vector<std::string> &addrs) {
    int port;
    std::string host;
    for (auto &addr : addrs) {
        if (UriUtils::parse_address(addr, &host, &port)) {
            addrs_.push_back(addr);
            auto channel = CreateChannel(host, port);
            auto stub = std::make_shared<NamingRpcService_Stub>(channel.get());
            channels_.emplace_back(channel);
            stubs_.emplace(addr, stub);
        }
    }
}

void NamingResolver::Stop() {
    if (channels_.empty()) return;

    for (auto channel : channels_) {
        channel->Run(ERROR_RPC_REQUESTCANCELED);
    }
    {
        std::unordered_map<std::string, StubPtr> empty;
        stubs_.swap(empty);
    }
    {
        std::vector<rpc::RpcChannelPtr> empty;
        channels_.swap(empty);
    }
}

NamingResolver::StubPtr NamingResolver::GetStub(const std::string &addr) {
    return stubs_[addr];
}

NamingResolver::StubPtr NamingResolver::GetStub(size_t addr_hint) {
    if (cached_addr_.length() > 0) {
        return stubs_[cached_addr_];
    }
    size_t index = addr_hint % addrs_.size();
    return stubs_[addrs_[index]];
}

rpc::RpcChannelPtr NamingResolver::CreateChannel(const std::string& ip, int port) {
    auto channel = std::make_shared<rpc::RpcChannel>(event_loop_);
    channel->Init(ip, port);
    return channel;
}

void NamingResolver::CacheAddr(const std::string& addr) {
    if (cached_addr_ == addr) return;

    if (addr.length() > 0 && stubs_.find(addr) == stubs_.end()) {
        std::string host;
        int port;
        if (UriUtils::parse_address(addr, &host, &port)) {
            auto channel = CreateChannel(host, port);
            channels_.push_back(channel);
            auto stub = std::make_shared<NamingRpcService_Stub>(channel.get());
            stubs_[addr] = stub;
        } else {
            log_error("%s bad address: %s", __FUNCTION__, addr.c_str());
            cached_addr_ = "";
            return;
        }
    }
    cached_addr_ = addr;
}

int NamingResolver::Put(const std::string &name, const std::string &value, uint32_t timeout, NamingCallback callback) {
    if (stubs_.size() == 0) {
        return ERROR_TNS_NOSTUB;
    }
    TnsContextPtr ctx = std::make_shared<TnsContext>();
    ctx->request.set_opcode(PUT_REQ);
    auto put_req = ctx->request.mutable_put_req();
    put_req->set_key(name);
    put_req->set_value(value);
    put_req->set_ttl(timeout);
    ctx->callback = std::move(callback);

    return Invoke(ctx);
}

int NamingResolver::Get(const std::string &name, NamingCallback callback) {
    if (stubs_.size() == 0) {
        return ERROR_TNS_NOSTUB;
    }
    TnsContextPtr ctx = std::make_shared<TnsContext>();
    ctx->request.set_opcode(GET_REQ);
    auto get_req = ctx->request.mutable_get_req();
    get_req->set_key(name);
    ctx->callback = std::move(callback);

    return Invoke(ctx);
}

int NamingResolver::Delete(const std::string &name, NamingCallback callback) {
    if (stubs_.size() == 0) {
        return ERROR_TNS_NOSTUB;
    }
    TnsContextPtr ctx = std::make_shared<TnsContext>();
    ctx->request.set_opcode(DEL_REQ);
    auto del_req = ctx->request.mutable_del_req();
    del_req->set_key(name);
    ctx->callback = std::move(callback);

    return Invoke(ctx);
}

int NamingResolver::Keys(const std::string &name, NamingCallback callback) {
    if (stubs_.size() == 0) {
        return ERROR_TNS_NOSTUB;
    }
    TnsContextPtr ctx = std::make_shared<TnsContext>();
    ctx->request.set_opcode(KEYS_REQ);
    auto keys_req = ctx->request.mutable_keys_req();
    keys_req->set_key(name);
    ctx->callback = std::move(callback);

    return Invoke(ctx);
}

int NamingResolver::Invoke(TnsContextPtr ctx) {
    ctx->controller.Reset();
    auto stub = GetStub(ctx->retryCount);
    if (!stub) {
        return ERROR_TNS_NOSTUB;
    }
    stub->Invoke(&ctx->controller, &ctx->request, &ctx->response,
                 ::google::protobuf::NewCallback(this, &NamingResolver::HandleInvoke, ctx));
    return ERROR_OK;
}

void NamingResolver::HandleInvoke(TnsContextPtr ctx) {
    NamingReply reply;
    if (ctx->controller.Failed()) {
        CacheAddr("");
        if (ctx->retryCount < kMaxRetryCount) {
            ++ctx->retryCount;
            Invoke(ctx);
            return;
        }
        reply.err = ctx->controller.ErrorCode();
        if (ctx->callback) {
            ctx->callback(reply);
        }
        return;
    }
    if (ctx->response.error_code() != ERROR_OK) {
        reply.err = ctx->response.error_code();
        if (ctx->response.error_code() == ERROR_TNS_SERVICEREDIRECT) {
            CacheAddr(ctx->response.redirect());
            if (ctx->redirectCount < kMaxRedirectCount) {
                ++ctx->redirectCount;
                Invoke(ctx);
                return;
            };
            if (ctx->callback) {
                ctx->callback(reply);
            }
            return;
        }
        if (ctx->callback) {
            ctx->callback(reply);
        }
        return;
    }
    if (ctx->retryCount) {
        CacheAddr(addrs_[ctx->retryCount % addrs_.size()]);
    }
    reply.err = ERROR_OK;
    switch (ctx->response.opcode()) {
    case GET_RES: {
        reply.type = NamingReplyType::GET;
        reply.value = ctx->response.get_res().value();
        break;
    }
    case PUT_RES: {
        reply.type = NamingReplyType::PUT;
        reply.value = ctx->response.get_res().value();
        break;
    }
    case DEL_RES: {
        reply.type = NamingReplyType::DEL;
        break;
    }
    case KEYS_RES: {
        reply.type = NamingReplyType::KEYS;
        auto& keys_res = ctx->response.keys_res();
        for (int i = 0; i < keys_res.keys_size(); ++i) {
            auto& key = keys_res.keys(i);
            reply.keys.push_back(key);
        }
        break;
    }
    default:
        reply.err = ERROR_TNS_METHODNOTFOUND;
        break;
    }
    if (ctx->callback) {
        ctx->callback(reply);
    }
    //call->controller.Trace();
}

}
}
