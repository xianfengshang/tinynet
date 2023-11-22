// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <tuple>
#include <string>
#include "base/string_view.h"
#include "base/io_buffer.h"
#ifdef _WIN32
#include <Ws2tcpip.h>
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#endif

#define  TCP_FLAGS_REUSEPORT 1

#define  TCP_FLAGS_IPV6ONLY 2

namespace NetUtils {

std::string* GetLocalIP(std::string* ip, int af = AF_INET);

int BindAndListenTcp(const char* host, int port,
                     int backlog, int flags, int* err);

int BindAndListenUnix(const char* path, int backlog, int* err);

int SetReuseAddr(int fd);

int SetReusePort(int fd);

int SetNonBlocking(int fd);

int SetNodelay(int fd);

int SetKeepAlive(int fd, int interval);

int GetSockError(int fd);

int GetSockName(int fd, std::string* ip, int* port);

int Close(int& fd);

int Accept(int s, std::string* ip, int* port, int* err);

int ConnectTcp(const char* host, int port, int af, int* err);

int ConnectUnix(const char* path, int* err);

int Read(int fd, void *buf, size_t len);

int Write(int fd, const void *data, size_t len);

int Readv(int fd, tinynet::iov_t* iov, int len);

int Writev(int fd, tinynet::iov_t* iov, int len);

int ReadAll(int fd, tinynet::iovs_t& iovs, int* nread);

int WriteAll(int fd, tinynet::iovs_t& iovs, int* nwrite);

int ReadAll(int fd, void *buf, size_t len, int* nread);

int WriteAll(int fd, const void *data, size_t len, int* nwrite);

int WaitEvent(int fd, int mask, int timeout_ms);

int LookupAddressFamily(const char* protocol);

int SocketPair(int family, int type, int protocol, int fd[2]);
}
