// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "logging/logging.h"
#include "http_client.h"
#include "util/string_utils.h"
#include "base/unique_id.h"
#include "base/at_exit.h"
#include "base/net_types.h"
#include "util/net_utils.h"

namespace tinynet {
namespace http {

const int kDefaultTaskTimeout = 30000;	//Default task timeout setting

struct HttpMethodReg {
    unsigned int uhash;	//hash code for uppercase
    unsigned int lhash;  //hash code for lowercase
    HttpMethod type;
    const char* name; //method name in uppercase
};
#define HTTP_METHOD_REG(UNAME, LNAME) \
{StringUtils::Hash(#UNAME), StringUtils::Hash(#LNAME), HTTP_##UNAME, #UNAME}

static HttpMethodReg HTTP_METHODS[] = {
    HTTP_METHOD_REG(GET, get),
    HTTP_METHOD_REG(POST, post),
    HTTP_METHOD_REG(HEAD, head),
    HTTP_METHOD_REG(OPTIONS, options),
    HTTP_METHOD_REG(PUT, put),
    HTTP_METHOD_REG(PATCH, patch),
    HTTP_METHOD_REG(DELETE, delete),
    HTTP_METHOD_REG(TRACE, trace),
    HTTP_METHOD_REG(CONNECT, connect),
    {0, 0, HTTP_GET, NULL}
};

static HttpMethod http_method_type(const char* name) {
    HttpMethod type = HTTP_GET;
    unsigned int hash = StringUtils::Hash(name);
    HttpMethodReg* reg;
    for (reg = HTTP_METHODS; reg->name; reg++) {
        if (hash == reg->uhash || hash == reg->lhash) {
            type = reg->type;
            break;
        }
    }
    return type;
}

static const char* http_method_name(HttpMethod type) {
    return HTTP_METHODS[type].name;
}

HttpClient::HttpContext::HttpContext(HttpClient* http_client, const client::Request& req, client::HttpCallback callback):
    guid_(0),
    http_client_(http_client),
    request_(req),
    cb_(std::move(callback)),
    headers_(0) {
    handle_ = curl_easy_init();
    guid_ = http_client_->event_loop_->NewUniqueId();
}

HttpClient::HttpContext::~HttpContext() {
    Dispose();
}

bool HttpClient::HttpContext::do_init() {
    std::string  url = get_url();
    curl_easy_setopt(handle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle_, CURLOPT_CLOSESOCKETFUNCTION, HttpClient::CurlCloseSocket);
    curl_easy_setopt(handle_, CURLOPT_CLOSESOCKETDATA, http_client_);
    curl_easy_setopt(handle_, CURLOPT_WRITEFUNCTION, CurlWrite);
    curl_easy_setopt(handle_, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(handle_, CURLOPT_HEADERFUNCTION, CurlHeader);
    curl_easy_setopt(handle_, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(handle_, CURLOPT_TIMEOUT_MS, request_.timeout);
    curl_easy_setopt(handle_, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(handle_, CURLOPT_COOKIEFILE, "");
    curl_easy_setopt(handle_, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle_, CURLOPT_AUTOREFERER, 1);
    curl_easy_setopt(handle_, CURLOPT_SSL_VERIFYPEER, 0);
    if (!request_.auth.username.empty()) {
        curl_easy_setopt(handle_, CURLOPT_USERNAME, request_.auth.username.c_str());
    }
    if (!request_.auth.password.empty()) {
        curl_easy_setopt(handle_, CURLOPT_PASSWORD, request_.auth.password.c_str());
    }
    if (!request_.proxy.url.empty()) {
        curl_easy_setopt(handle_, CURLOPT_PROXY, request_.proxy.url.c_str());
        if (!request_.proxy.auth.username.empty()) {
            curl_easy_setopt(handle_, CURLOPT_PROXYUSERNAME, request_.proxy.auth.username.c_str());
        }
        if (!request_.proxy.auth.password.empty()) {
            curl_easy_setopt(handle_, CURLOPT_PROXYPASSWORD, request_.proxy.auth.password.c_str());
        }
    }
    if (request_.withCredentials) {
        std::string cookie_data;
        for (auto &cookie : request_.cookies) {
            cookie_data.append(cookie.first);
            cookie_data.append(": ");
            cookie_data.append(cookie.second);
        }
        curl_easy_setopt(handle_, CURLOPT_COOKIE, cookie_data.c_str());
    }
    if (request_.maxRedirects)
        curl_easy_setopt(handle_, CURLOPT_MAXREDIRS, request_.maxRedirects);
    if (request_.verbose)
        curl_easy_setopt(handle_, CURLOPT_VERBOSE, 1);

    if (request_.headers.size() > 0) {
        char buffer[256] = { 0 };
        for (auto &item : request_.headers) {
            snprintf(buffer, sizeof(buffer), "%s: %s", item.first.c_str(), item.second.c_str());
            headers_ = curl_slist_append(headers_, buffer);
        }
        curl_easy_setopt(handle_, CURLOPT_HTTPHEADER, headers_);
    }
    if (request_.verbose) {
        log_info("%s", __FUNCTION__);
    }
    return true;
}

bool HttpClient::HttpContext::do_get() {
    if (request_.verbose) {
        log_info("%s", __FUNCTION__);
    }
    return true;
}

bool HttpClient::HttpContext::do_post() {
    curl_easy_setopt(handle_, CURLOPT_CUSTOMREQUEST, http_method_name(HTTP_POST));
    if (request_.data.length() > 0) {
        curl_easy_setopt(handle_, CURLOPT_POST, 1);
        curl_easy_setopt(handle_, CURLOPT_POSTFIELDS, request_.data.c_str());
    }

    if (request_.verbose) {
        log_info("%s", __FUNCTION__);
    }
    return true;
}

bool HttpClient::HttpContext::do_head() {
    curl_easy_setopt(handle_, CURLOPT_CUSTOMREQUEST, http_method_name(HTTP_HEAD));
    if (request_.verbose) {
        log_info("%s", __FUNCTION__);
    }
    return true;
}

bool HttpClient::HttpContext::do_options() {
    curl_easy_setopt(handle_, CURLOPT_CUSTOMREQUEST, http_method_name(HTTP_OPTIONS));
    if (request_.verbose) {
        log_info("%s", __FUNCTION__);
    }
    return true;
}


bool HttpClient::HttpContext::do_put() {
    curl_easy_setopt(handle_, CURLOPT_CUSTOMREQUEST, http_method_name(HTTP_PUT));
    if (request_.verbose) {
        log_info("%s", __FUNCTION__);
    }
    return true;
}

bool HttpClient::HttpContext::do_patch() {
    curl_easy_setopt(handle_, CURLOPT_CUSTOMREQUEST, http_method_name(HTTP_PATCH));
    if (request_.verbose) {
        log_info("%s", __FUNCTION__);
    }
    return true;
}

bool HttpClient::HttpContext::do_delete() {
    curl_easy_setopt(handle_, CURLOPT_CUSTOMREQUEST, http_method_name(HTTP_DELETE));
    if (request_.verbose) {
        log_info("%s", __FUNCTION__);
    }
    return true;
}

bool HttpClient::HttpContext::do_trace() {
    curl_easy_setopt(handle_, CURLOPT_CUSTOMREQUEST, http_method_name(HTTP_TRACE));
    if (request_.verbose) {
        log_info("%s", __FUNCTION__);
    }
    return true;
}

bool HttpClient::HttpContext::do_connect() {
    curl_easy_setopt(handle_, CURLOPT_CUSTOMREQUEST, http_method_name(HTTP_CONNECT));
    if (request_.verbose) {
        log_info("%s", __FUNCTION__);
    }
    return true;
}

bool HttpClient::HttpContext::Send() {
    HttpMethod method = http_method_type(request_.method.c_str());

    bool result = do_init();
    switch (method) {
    case HTTP_GET:
        result = do_get();
        break;
    case HTTP_POST:
        result = do_post();
        break;
    case HTTP_HEAD:
        result = do_head();
        break;
    case HTTP_OPTIONS:
        result = do_options();
        break;
    case HTTP_PUT:
        result = do_put();
        break;
    case HTTP_PATCH:
        result = do_patch();
        break;
    case HTTP_DELETE:
        result = do_delete();
        break;
    case HTTP_TRACE:
        result = do_trace();
        break;
    case HTTP_CONNECT:
        result = do_connect();
        break;
    default:
        result = false;
        break;
    }
    if (result) {
        CURLMcode mCode = curl_multi_add_handle(http_client_->get_multi_handle(), handle_);
        if (mCode != CURLM_OK) {
            log_error("curl_multi_add_handle failed, curlHandle[%p], handle[%p], code[%d], msg[%s]!",
                      http_client_->get_multi_handle(), handle_, mCode, curl_multi_strerror(mCode));
            result = false;
        }
    }
    return result;
}
std::string HttpClient::HttpContext::get_url() {
    if (StringUtils::StartsWith(request_.url, "http")) {
        return request_.url;
    }
    if (request_.baseUrl.empty()) {
        return request_.url;
    }
    std::string uri = request_.baseUrl;
    if (!StringUtils::EndsWith(uri, "/")) {
        uri.append("/");
    }
    uri.append(request_.url);

    if (request_.method == "GET" && !request_.params.empty()) {
        std::string query_param = "?";
        size_t i = 0;
        for (auto &param : request_.params) {
            query_param.append(param.first);
            query_param.append("=");
            query_param.append(param.second);
            ++i;
            if (i < request_.params.size()) {
                query_param.append("&");
            }
        }
    }
    return uri;
}

size_t HttpClient::HttpContext::CurlWrite(void *buffer, size_t size, size_t nmemb, void *ud) {
    auto self = static_cast<HttpClient::HttpContext*>(ud);
    self->WriteData(buffer, size * nmemb);
    return size * nmemb;
}

size_t HttpClient::HttpContext::CurlHeader(void *buffer, size_t size, size_t nmemb, void *ud) {
    auto self = static_cast<HttpClient::HttpContext*>(ud);
    self->WriteHeader(buffer, size * nmemb);
    return size * nmemb;
}

void HttpClient::HttpContext::Run(CURLcode code) {
    response_.code = code;
    response_.msg = curl_easy_strerror(code);
    curl_easy_getinfo(handle_, CURLINFO_RESPONSE_CODE, &response_.status);
    response_.guid = get_guid();
    cb_(response_);
    http_client_->RemoveContext(handle_);
}

void HttpClient::HttpContext::Dispose() {
    if (headers_) {
        curl_slist_free_all(headers_);
        headers_ = 0;
    }
    if (handle_) {
        curl_multi_remove_handle(http_client_->multi_handle_, handle_);
        curl_easy_cleanup(handle_);
        handle_ = 0;
    }

}

void HttpClient::HttpContext::AddEvent(int fd, int events) {
    http_client_->event_loop_->AddEvent(fd, events, std::bind(&HttpClient::HttpContext::HandleEvent, this, std::placeholders::_1, std::placeholders::_2));
}

void HttpClient::HttpContext::ClearEvent(int fd) {
    http_client_->event_loop_->ClearEvent(fd, net::EVENT_FULL_MASK);
}

void HttpClient::HttpContext::HandleEvent(int fd, int events) {
    int runningHandles;
    int flags = 0;
    if (events & net::EVENT_READABLE)
        flags |= CURL_CSELECT_IN;
    if (events & net::EVENT_WRITABLE)
        flags |= CURL_CSELECT_OUT;
    if (events & net::EVENT_ERROR)
        flags |= CURL_CSELECT_ERR;
#ifdef _WIN32
    if (events & net::EVENT_READABLE) {
        AddEvent(fd, net::EVENT_READABLE);
    }
#endif
    CURLMcode mCode = curl_multi_socket_action(http_client_->get_multi_handle(), fd, flags,
                      &runningHandles);
    if (mCode != CURLM_OK) {
        log_error("curl_multi_socket_action failed, handle[%p], sockfd[%d], flags[%d], code[%d], msg[%s]",
                  http_client_->get_multi_handle(), fd, flags, mCode, curl_multi_strerror(mCode));
    }
    auto self = shared_from_this();
    self->http_client_->Update();
}

std::once_flag HttpClient::once_flag_;

void HttpClient::Initialize() {
    CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != CURLE_OK) {
        log_fatal("curl_global_init failed, code[%d], msg[%s]", code, curl_easy_strerror(code));
    }
    tinynet::AtExitManager::RegisterCallback(Finalize, nullptr);
}

void HttpClient::Finalize(void*) {
    curl_global_cleanup();
}

HttpClient::HttpClient(EventLoop *loop):
    event_loop_(loop),
    context_dict_(),
    multi_handle_(NULL),
    timer_guid_(INVALID_TIMER_ID) {

}

HttpClient::~HttpClient() {
    if (timer_guid_) {
        event_loop_->ClearTimer(timer_guid_);
    }
    if (multi_handle_) {
        curl_multi_cleanup(multi_handle_);
        multi_handle_ = nullptr;
    }
}

void HttpClient::Init() {
    std::call_once(once_flag_, Initialize);
    multi_handle_ = curl_multi_init();
    curl_multi_setopt(multi_handle_, CURLMOPT_SOCKETFUNCTION, CurlSocket);
    curl_multi_setopt(multi_handle_, CURLMOPT_TIMERFUNCTION, CurlTimer);
    curl_multi_setopt(multi_handle_, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(multi_handle_, CURLMOPT_TIMERDATA, this);
}

void HttpClient::Update() {
    if (context_dict_.empty()) return;
    CurlRead();
}

void HttpClient::Stop() {
    if (timer_guid_) {
        event_loop_->ClearTimer(timer_guid_);
    }

    if (multi_handle_) {
        curl_multi_cleanup(multi_handle_);
        multi_handle_ = nullptr;
    }

}

void HttpClient::CurlRead() {
    CURLMsg *msg;
    CURL* easyHandle;
    int pending;
    while ((msg = curl_multi_info_read(multi_handle_, &pending))) {
        switch (msg->msg) {
        case CURLMSG_DONE: {
            easyHandle = msg->easy_handle;
            HttpContextPtr ctx = GetContext(easyHandle);
            if (ctx) {
                ctx->Run(msg->data.result);
            } else {
                curl_multi_remove_handle(multi_handle_, easyHandle);
                curl_easy_cleanup(easyHandle);
                log_error("Cant not find http context, handle[%p].", easyHandle);
            }
            break;
        }
        default:
            break;
        }
    }
}

HttpClient::HttpContextPtr HttpClient::GetContext(CURL* handle) {
    auto it = context_dict_.find(handle);
    if (it == context_dict_.end()) {
        return nullptr;
    }
    return it->second;
}

HttpClient::HttpContextPtr HttpClient::CreateContext(const client::Request& req, client::HttpCallback callback) {
    auto ctx = std::make_shared<HttpClient::HttpContext>(this, req, callback);
    context_dict_.emplace(ctx->get_handle(), ctx);
    return ctx;
}

void HttpClient::RemoveContext(CURL* handle) {
    context_dict_.erase(handle);
}

int HttpClient::CurlTimer(void *handle, long timeout_ms, void *userp) {
    auto self = static_cast<HttpClient*>(userp);
    if (!self) return -1;
    if (self->timer_guid_) {
        self->event_loop_->ClearTimer(self->timer_guid_);
    }
    if (timeout_ms >= 0) {
        self->timer_guid_ = self->event_loop_->AddTimer(timeout_ms, 0, std::bind(&HttpClient::CurlTimeout, self));
    }
    return 0;
}


void HttpClient::CurlTimeout() {
    int runningHandles = 0;
    CURLMcode code = curl_multi_socket_action(multi_handle_, CURL_SOCKET_TIMEOUT, 0,
                     &runningHandles);
    if (code != CURLM_OK) {
        log_error("curl_multi_socket_action failed, handle[%p], sockfd[%d], flags[%d], code[%d], msg[%s]",
                  multi_handle_, CURL_SOCKET_TIMEOUT, 0, code, curl_multi_strerror(code));
    }
    Update();
}

int HttpClient::CurlSocket(CURL* easy_handle, curl_socket_t s, int action, void *userp, void * socketp) {
    HttpClient *self = static_cast<HttpClient*>(userp);
    if (!self) return -1;
    HttpContextPtr ctx = self->GetContext(easy_handle);
    if (!ctx) {
        log_error("on_socket_action, can not find request. easyHandle[%p], s[%d], action[%d], userp[%p], socketp[%p]",
                  easy_handle, s, action, userp, socketp);
        return -1;
    }
    if (socketp == 0) {
        curl_multi_assign(self->multi_handle_, s, easy_handle);
    }
    int events = 0;
    if (action == CURL_POLL_REMOVE) {
        events = 0;
    } else if (action == CURL_POLL_IN) {
        events |= net::EVENT_READABLE;
    } else if (action == CURL_POLL_OUT) {
        events |= net::EVENT_WRITABLE;
    } else {
        events |= net::EVENT_READABLE;
        events |= net::EVENT_WRITABLE;
    }
    if (events) {
        ctx->AddEvent((int)s, events);
    } else {
        ctx->ClearEvent((int)s);
    }
    return 0;
}

int HttpClient::CurlCloseSocket(void *clientp, curl_socket_t s) {
    HttpClient *self = static_cast<HttpClient*>(clientp);
    int fd = (int)s;
    if (self) {
        self->event_loop_->ClearEvent(fd, net::EVENT_FULL_MASK);
    }
    NetUtils::Close(fd);
    return 0;
}

int64_t HttpClient::Request(const client::Request& req, client::HttpCallback callback) {
    HttpContextPtr ctx = CreateContext(req, std::move(callback));
    if (!ctx) return 0;
    if (ctx->Send()) {
        return ctx->get_guid();
    }
    RemoveContext(ctx->get_handle());
    return 0;
}

int64_t HttpClient::Get(const std::string &url, client::HttpCallback callback) {
    client::Request req;
    req.url = url;
    req.method = "GET";
    req.timeout = kDefaultTaskTimeout;
    return Request(req, std::move(callback));
}

int64_t HttpClient::Post(const std::string &url, const std::string& data, client::HttpCallback callback) {
    client::Request req;
    req.url = url;
    req.method = "POST";
    req.timeout = kDefaultTaskTimeout;
    req.data = data;
    return Request(req, std::move(callback));
}
}
}
