// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "net/event_loop.h"
#include "mysql_types.h"
#include "mysql_query.h"
#include "mysql_channel.h"
#include "base/string_view.h"
#include "base/spinlock.h"
#include <queue>
namespace mysql {

using MysqlChannelPtr = std::unique_ptr<MysqlChannel>;

class MysqlClient  {
  public:
    MysqlClient(tinynet::EventLoop *loop);
    ~MysqlClient();
  public:
    void Init(const mysql::Config &config);
    void Shutdown();
    int64_t Query(const char* data, size_t len, MysqlCallback callback);
  private:
    void HandleRequest(MysqlQueryPtr query);
    void HandleResponse(MysqlQueryPtr query);
    MysqlQueryPtr PeekQuery();
  private:
    //Process request queue
    void RunReq();
    //Process response queue
    void RunResp();
  private:
    typedef std::queue<MysqlQueryPtr> QueryQueue;
  private:
    tinynet::EventLoop *main_loop_;
    tinynet::EventLoop *event_loop_;
    std::vector<MysqlChannelPtr> channels_;
    QueryQueue	        request_queue_;
    QueryQueue	        response_queue_;
    tinynet::SpinLock	request_lock_;
    tinynet::SpinLock	response_lock_;
    std::unique_ptr<tinynet::EventLoop> thread_loop_;
    std::unique_ptr<std::thread> thread_;
};
}
