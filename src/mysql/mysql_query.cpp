// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "mysql_query.h"
namespace mysql {
MysqlQuery::MysqlQuery(uint64_t guid, MysqlCallback callback) :
    guid_(guid),
    callback_(std::move(callback)) {
}

void MysqlQuery::Run() {
    if (callback_) {
        callback_(response_);
    }
}
}
