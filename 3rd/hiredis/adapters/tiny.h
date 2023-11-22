#pragma once
#ifndef __HIREDIS_TINY_H__
#define __HIREDIS_TINY_H__
#include "hiredis/async.h"
#include "net/event_loop.h"
#include "util/net_utils.h"

class RedisTinyAdapter {
  private:
    static void RedisTinyAddRead(void * adapter) {
        RedisTinyAdapter * a = static_cast<RedisTinyAdapter *>(adapter);
        a->addRead();
    }

    static void RedisTinyDelRead(void * adapter) {
        RedisTinyAdapter * a = static_cast<RedisTinyAdapter *>(adapter);
        a->delRead();
    }

    static void RedisTinyAddWrite(void * adapter) {
        RedisTinyAdapter * a = static_cast<RedisTinyAdapter *>(adapter);
        a->addWrite();
    }

    static void RedisTinyDelWrite(void * adapter) {
        RedisTinyAdapter * a = static_cast<RedisTinyAdapter *>(adapter);
        a->delWrite();
    }

    static void RedisTinyCleanup(void * adapter) {
        RedisTinyAdapter * a = static_cast<RedisTinyAdapter *>(adapter);
        a->cleanup();
    }

  public:
    RedisTinyAdapter(tinynet::EventLoop* loop)
        : m_loop(loop), m_ctx(0) { }

    ~RedisTinyAdapter() = default;

    int setContext(redisAsyncContext * ac) {
        if (ac->ev.data != NULL) {
            return REDIS_ERR;
        }
        m_ctx = ac;
        m_ctx->ev.data = this;
        m_ctx->ev.addRead = RedisTinyAddRead;
        m_ctx->ev.delRead = RedisTinyDelRead;
        m_ctx->ev.addWrite = RedisTinyAddWrite;
        m_ctx->ev.delWrite = RedisTinyDelWrite;
        m_ctx->ev.cleanup = RedisTinyCleanup;
        return REDIS_OK;
    }

    redisAsyncContext* createContext(const char* host, int port) {
        redisAsyncContext* ac = redisAsyncConnect(host, port);
        if (ac != NULL) {
            setContext(ac);
        }
        return ac;
    }
  private:
    void addRead() {
        m_loop->AddEvent((int)m_ctx->c.fd, tinynet::net::EVENT_READABLE,
                         std::bind(&RedisTinyAdapter::handleEvent, this, std::placeholders::_1, std::placeholders::_2));
    }

    void delRead() {
        m_loop->ClearEvent((int)m_ctx->c.fd, tinynet::net::EVENT_READABLE);
    }

    void addWrite() {
        m_loop->AddEvent((int)m_ctx->c.fd, tinynet::net::EVENT_WRITABLE,
                         std::bind(&RedisTinyAdapter::handleEvent, this, std::placeholders::_1, std::placeholders::_2));
    }

    void delWrite() {
        m_loop->ClearEvent((int)m_ctx->c.fd, tinynet::net::EVENT_WRITABLE);
    }

    void cleanup() {
        m_loop->ClearEvent((int)m_ctx->c.fd, tinynet::net::EVENT_FULL_MASK);
    }

  private:
    void handleEvent(int fd, int events) {
        if (events & tinynet::net::EVENT_READABLE) {
            redisAsyncHandleRead(m_ctx);
        }
        if (events & tinynet::net::EVENT_WRITABLE) {
            redisAsyncHandleWrite(m_ctx);
        }
    }
  private:
    tinynet::EventLoop* m_loop;
    redisAsyncContext * m_ctx;
};

#endif /* !__HIREDIS_TINY_H__ */
