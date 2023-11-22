// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <unordered_map>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <limits>
#include "curl.h"
#include "base/clock.h"
#include "net/event_loop.h"

namespace tinynet {
namespace http {
namespace client {
/*
* Auth config
*/
struct AuthConfig {
    std::string username;
    std::string password;
};
/*
* Proxy config
*/
struct ProxyConfig {
    std::string url;
    AuthConfig auth;
};
/*
* Request config
*/
struct Request {
    std::string url;
    std::string method{ "GET" };
    std::string baseUrl;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> cookies;
    std::string data;
    int timeout{ 0 };
    bool withCredentials{ false };
    AuthConfig auth;
    ProxyConfig proxy;
    int maxContentLength{ 1024*1024 };
    int maxRedirects{ 5 };
    bool verbose{ false };
};

/*
*	@brief Response Schema
*/
struct Response {
    int64_t guid {0};
    int code {0};
    std::string msg;
    std::string data;
    long status;
    std::string headers;
    //std::map<std::string, std::string> headers;
    std::vector<std::string> cookies;
};
typedef std::map<std::string, std::string> Headers;

typedef std::function<void(const client::Response &response)> HttpCallback;
}

enum HttpMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_PUT,
    HTTP_PATCH,
    HTTP_DELETE,
    HTTP_TRACE,
    HTTP_CONNECT
};

class HttpClient {
  public:
    HttpClient(EventLoop *loop);
    ~HttpClient();
  public:
    void Init();
    void Update();
    void Stop();
  public:
    int64_t Request(const client::Request& req, client::HttpCallback callback);
    int64_t Get(const std::string &url, client::HttpCallback callback);
    int64_t Post(const std::string &url, const std::string& data, client::HttpCallback callback);
  public:
    class HttpContext:
        public std::enable_shared_from_this<HttpContext> {
        friend class HttpClient;
      public:
        HttpContext(HttpClient* http_client, const client::Request& req, client::HttpCallback callback);
        ~HttpContext();
      public:
        bool Send();
        void Run(CURLcode code);
      private:
        bool do_init();
        bool do_get();
        bool do_post();
        bool do_head();
        bool do_options();
        bool do_put();
        bool do_patch();
        bool do_delete();
        bool do_trace();
        bool do_connect();
      private:
        void Dispose();

        void AddEvent(int fd, int events);

        void ClearEvent(int fd);

        void HandleEvent(int fd, int events);
      private:
        std::string get_url();

        int64_t get_guid() const { return guid_; }

        CURL* get_handle() const { return handle_; }
      private:
        static size_t CurlWrite(void *buffer, size_t size, size_t nmemb, void *ud);

        static size_t CurlHeader(void *buffer, size_t size, size_t nmemb, void *ud);

        void WriteData(void *data, size_t len) { response_.data.append((char*)(data), len); }

        void WriteHeader(void *data, size_t len) { response_.headers.append((char*)(data), len); }

      private:
        int64_t			guid_;
        CURL*	        handle_;
        HttpClient*	http_client_;
        client::Request	    request_;
        client::Response    response_;
        client::HttpCallback	cb_;
        struct curl_slist *headers_;
    };
    using HttpContextPtr = std::shared_ptr<HttpContext>;
  public:
    EventLoop* get_event_loop() { return event_loop_; }
    CURLM* get_multi_handle() { return multi_handle_; }
  private:
    static int CurlTimer(void *handle,long timeout_ms, void *userp);
    static int CurlSocket(CURL *easy_handle, curl_socket_t s, int action, void *userp, void * socketp);
    static int CurlCloseSocket(void *clientp, curl_socket_t s);
    void CurlTimeout();
    void CurlRead();

    HttpContextPtr GetContext(CURL* handle);
    HttpContextPtr CreateContext(const client::Request& req, client::HttpCallback callback);
    void RemoveContext(CURL* handle);
  private:
    static std::once_flag once_flag_;
    static void Initialize();
    static void Finalize(void*);
  public:
    using ContextDict = std::unordered_map<CURL*, HttpContextPtr>;
  private:
    EventLoop *    event_loop_;
    ContextDict     context_dict_;
    CURLM*			multi_handle_;
    TimerId			timer_guid_;
};
}
}
