// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "rpc_info.h"
#include "rpc_controller.h"
#include "rpc_helper.h"
namespace tinynet {
namespace rpc {
RpcInfo::RpcInfo(uint64_t seq, google::protobuf::RpcController *controller, const google::protobuf::Message *request,
                 google::protobuf::Message *response, google::protobuf::Closure *done):
    seq_(seq),
    controller_(controller),
    request_(request),
    response_(response),
    done_(done),
    delete_members_(false) {
}


RpcInfo::RpcInfo(uint64_t seq, const google::protobuf::Message* request, google::protobuf::Message *response):
    seq_(seq),
    controller_(new (std::nothrow ) rpc::RpcController()),
    request_(request),
    response_(response),
    done_(nullptr),
    delete_members_(true) {

}

RpcInfo::~RpcInfo() {
    if (delete_members_) {
        if (controller_) {
            delete controller_;
            controller_ = nullptr;
        }
        if (request_) {
            delete request_;
            request_ = nullptr;
        }
        if (response_) {
            delete response_;
            response_ = nullptr;
        }
    }
    if (done_) {
        delete done_;
        done_ = nullptr;
    }
}

void RpcInfo::Run(int err) {
    auto c = static_cast<RpcController*>(controller_);
    if (c && err) {
        c->SetFailed(err);
    }
    ClosureGuard guard(done_);
    done_ = nullptr;
}
}
}
