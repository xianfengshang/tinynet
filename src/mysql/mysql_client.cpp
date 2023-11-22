// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "mysql_client.h"
#include "mysql_channel.h"
#include <functional>
#include "base/at_exit.h"
#include "base/error_code.h"
#include "logging/logging.h"
namespace mysql {

MysqlClient::MysqlClient(tinynet::EventLoop *loop) :
    main_loop_(loop),
    event_loop_(nullptr) {
}

MysqlClient::~MysqlClient() = default;

void MysqlClient::Init(const mysql::Config &config) {
    if (config.use_io_thread) {
        // 1+ setup event loop
        thread_loop_.reset(new(std::nothrow) EventLoop());
        thread_loop_->Init();

        // 2+ setup id worker
        thread_loop_->SetIdAllocator(tinynet::IdAllocator::Instance(), false);

        event_loop_ = thread_loop_.get();
    } else {
        event_loop_ = main_loop_;
    }

    for (int i = 0; i < config.connectionLimit; ++i) {
        auto channel = new (std::nothrow) MysqlChannel(event_loop_);
        if (channel == nullptr) {
            log_error("Failed to new mysql channel, maybe out of memory");
            return;
        }
        channel->Init(config);
        channels_.emplace_back(channel);
    }
    if (config.use_io_thread) {
        thread_.reset(new(std::nothrow) std::thread(&tinynet::EventLoop::Run, thread_loop_.get(), tinynet::EventLoop::RUN_FOREVER));
        if (!thread_) {
            log_error("Failed to new thread, maybe out of memory");
        }
    }
}

void MysqlClient::Shutdown() {
    if (thread_loop_) {
        thread_loop_->Exit();
        thread_->join();
    }

    channels_.clear();
}

int64_t MysqlClient::Query(const char* data, size_t len, MysqlCallback callback) {
    auto guid = main_loop_->NewUniqueId();
    auto query = std::make_shared<MysqlQuery>(guid, std::move(callback));
    auto &request = query->get_request();
    request.sql.assign(data, len);
    if (channels_.empty()) {
        auto &response = query->get_response();
        response.code = tinynet::ERROR_MYSQL_UNINITIALIZED;
        response.msg = tinynet_strerror(response.code);
        HandleResponse(query);
    } else {
        HandleRequest(query);
    }
    return guid;
}

void MysqlClient::RunReq() {
    for (auto &channel : channels_) {
        if (channel->is_querying())
            continue;
        auto query = PeekQuery();
        if (!query)
            break;
        channel->Query(query, std::bind(&MysqlClient::HandleResponse, this, std::placeholders::_1));
    }
}

void MysqlClient::RunResp() {
    QueryQueue response_queue;
    {
        std::lock_guard<tinynet::SpinLock> lock(response_lock_);
        response_queue_.swap(response_queue);
    }
    while (response_queue.size() > 0) {
        auto query = response_queue.front();
        response_queue.pop();
        query->Run();
    }
}

void MysqlClient::HandleRequest(MysqlQueryPtr query) {
    {
        std::lock_guard<tinynet::SpinLock> lock(request_lock_);
        request_queue_.emplace(std::move(query));
    }
    event_loop_->AddTask(std::bind(&MysqlClient::RunReq, this));
}

void MysqlClient::HandleResponse(MysqlQueryPtr query) {
    {
        std::lock_guard<tinynet::SpinLock> lock(response_lock_);
        response_queue_.emplace(std::move(query));
    }
    main_loop_->AddTask(std::bind(&MysqlClient::RunResp, this));
    event_loop_->AddTask(std::bind(&MysqlClient::RunReq, this));
}

MysqlQueryPtr MysqlClient::PeekQuery() {
    std::lock_guard<tinynet::SpinLock> lock(request_lock_);
    if (request_queue_.empty())
        return nullptr;
    auto task = request_queue_.front();
    request_queue_.pop();
    return task;
}

}
