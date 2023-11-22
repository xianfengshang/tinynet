// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <mutex>
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/engine.h"
#include "openssl/conf.h"

namespace tinynet {
class SSLContext  {
  public:
    struct Options {
        std::string ssl_key;
        std::string ssl_cert;
        std::string ssl_ca;
        std::string ssl_capath;
    };
  public:
    SSLContext();
    ~SSLContext();
  public:
    bool Init(const Options& opts);
  public:
    SSL_CTX* get_ctx() { return ctx_; }
  public:
    static std::once_flag once_flag_;
    static void Initialize();
    static void Finalize(void*);
  private:
    SSL_CTX* ctx_;
};
}