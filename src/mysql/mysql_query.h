// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "mysql_types.h"
#include <functional>
#include <memory>

namespace mysql {

typedef std::function<void(const QueryResponse& results)> MysqlCallback;

class MysqlQuery {
  public:
    MysqlQuery(uint64_t guid, MysqlCallback callback);
  public:
    void Run();
  public:
    int64_t get_guid() const { return guid_; }
    QueryRequest& get_request() { return request_; }
    QueryResponse& get_response() { return response_; }
  private:
    int64_t guid_;
    QueryRequest request_;
    QueryResponse response_;
    MysqlCallback callback_;
};
typedef std::shared_ptr<MysqlQuery> MysqlQueryPtr;
}
