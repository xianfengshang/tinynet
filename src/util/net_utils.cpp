// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "util/net_utils.h"
#include "base/error_code.h"
#include "base/winsock_manager.h"
#include "base/net_types.h"
#include "util/fs_utils.h"
#include "util/uri_utils.h"
#include "util/string_utils.h"
#include <stdio.h>
#if defined(_WIN32)
#include <mstcpip.h>
#include <iphlpapi.h>
#include <WS2tcpip.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <poll.h>
#endif

namespace NetUtils {
#ifdef _WIN32
static const size_t kMaxUnixPathFileLength = 64;
#endif

#ifdef _WIN32

std::string* GetLocalIP(std::string* ip, int af) {
    ULONG ulRetVal, ulSize;
    PIP_ADAPTER_ADDRESSES begin, end, it;
    PIP_ADAPTER_UNICAST_ADDRESS ua;
    char buf[INET6_ADDRSTRLEN];
    ulRetVal = ulSize = 0;
    memset(buf, 0, sizeof(buf));
    ulRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &ulSize);
    if (ulRetVal != ERROR_BUFFER_OVERFLOW) {
        return NULL;
    }
    begin = (PIP_ADAPTER_ADDRESSES)malloc(ulSize);
    end = NULL;
    ulRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, begin, &ulSize);
    if (ulRetVal != NO_ERROR) {
        free(begin);
        return NULL;
    }
    for (it = begin; it != end; it = it->Next) {
        if (it->IfType != MIB_IF_TYPE_ETHERNET && it->IfType != IF_TYPE_IEEE80211) continue;
        for (ua = it->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
            struct sockaddr* sa = ua->Address.lpSockaddr;
            if (af == AF_UNSPEC || sa->sa_family == af) {
                if (getnameinfo(sa, ua->Address.iSockaddrLength, buf, sizeof(buf), NULL, 0, NI_NUMERICHOST) == 0) {
                    ip->assign(buf);
                    break;
                }
            }
        }
        if (!ip->empty()) break;
    }
    free(begin);
    return ip;
}
#else
std::string* GetLocalIP(std::string* ip) {
    return GetLocalIP(ip, AF_INET);
}
std::string* GetLocalIP(std::string* ip, int af) {
    struct ifaddrs *begin, *end, *it;
    if (getifaddrs(&begin) == -1) {
        return NULL;
    }
    end = NULL;
    char buf[INET6_ADDRSTRLEN];
    for (it = begin; it != end; it = it->ifa_next) {
        if (it->ifa_addr == NULL ||
                (it->ifa_flags & IFF_UP) == 0 ||
                (it->ifa_flags & IFF_RUNNING) == 0 ||
                (it->ifa_flags & IFF_LOOPBACK)) {
            continue;
        }
        if (af == AF_UNSPEC || it->ifa_addr->sa_family == af) {
            if (getnameinfo(it->ifa_addr, sizeof(struct sockaddr_in), buf, sizeof(buf), NULL, 0, NI_NUMERICHOST) == 0) {
                ip->assign(buf);
                break;
            }
        }
    }
    freeifaddrs(begin);
    return ip;
}
#endif
static int SetIPV6Only(int s, int optval) {
    if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&optval, sizeof(optval)) == -1) {
        return tinynet::ERROR_SOCKET_SETIPV6ONLY;
    }
    return 0;
}

static int Listen(int s, int backlog) {
#ifdef _WIN32
    if (tinynet::net::WinsockManager::Listen(s, backlog) == SOCKET_ERROR) {
        return tinynet::ERROR_SOCKET_LISTEN;
    }
#else
    if (::listen(s, backlog) != 0) {
        return tinynet::ERROR_SOCKET_LISTEN;
    }
#endif
    return 0;
}

static int Bind(int s, const sockaddr* addr, int namelen) {
    if (::bind(s, addr, namelen) != 0) {
        return tinynet::ERROR_SOCKET_BIND;
    }
    return 0;
}

int BindAndListenTcp(const char* host, int port, int backlog, int flags, int* err) {
    int s, res;
    char service[6];
    struct addrinfo hints, *begin, *end, *it;
    snprintf(service, sizeof(service), "%d", port);
    service[sizeof(service) - 1] = '\0';
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    end = NULL;
    res = getaddrinfo(host, service, &hints, &begin);
    if (res != 0) {
        *err = tinynet::ERROR_NET_GETADDRINFO;
        return -1;
    }
    *err = tinynet::ERROR_SOCKET_BINDANDLISTEN;
    for (it = begin; it != end; it = it->ai_next) {
        s = (int)::socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (s == -1) continue;
        if ((*err = SetNonBlocking(s)) != 0) break;
        if ((*err = SetReuseAddr(s)) != 0) break;
        if (it->ai_family == AF_INET6) {
            if ((*err = SetIPV6Only(s, flags & TCP_FLAGS_IPV6ONLY)) != 0) break;
        }
        if (flags & TCP_FLAGS_REUSEPORT) {
            if ((*err = SetReusePort(s)) != 0) break;
        }
        if ((*err = Bind(s, it->ai_addr, (int)it->ai_addrlen)) == 0) {
            break;
        } else {
            Close(s);
        }
    }
    freeaddrinfo(begin);
    if (*err) {
        return -1;
    }
    if ((*err = Listen(s, backlog)) != 0) {
        Close(s);
        return -1;
    }
    return s;
}

#ifdef _WIN32
int BindAndListenUnix(const char* path, int backlog, int *err) {
    *err = 0;
    if (FileSystemUtils::exists(path)) {
        *err = tinynet::ERROR_SOCKET_BINDANDLISTEN;
        return -1;
    }
    int s = BindAndListenTcp("127.0.0.1", 0, backlog, 0, err);
    if (s == -1) return -1;
    int port;
    if (GetSockName(s, NULL, &port) == -1) {
        Close(s);
        *err = tinynet::ERROR_SOCKET_BINDANDLISTEN;
        return -1;
    }
    std::wstring wpath;
    if (!StringUtils::convert_utf8_to_utf16(path, &wpath)) {
        Close(s);
        *err = tinynet::ERROR_SOCKET_BINDANDLISTEN;
        return -1;
    }
    HANDLE hfile = CreateFileW(wpath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) {
        Close(s);
        *err = tinynet::ERROR_SOCKET_BINDANDLISTEN;
        return -1;
    }
    char addr[kMaxUnixPathFileLength];
    DWORD len = snprintf(addr, sizeof(addr), "tcp://127.0.0.1:%d", port);
    if (!WriteFile(hfile, addr, len, &len, NULL)) {
        CloseHandle(hfile);
        Close(s);
        *err = tinynet::ERROR_SOCKET_BINDANDLISTEN;
        return -1;
    }
    FlushFileBuffers(hfile);
    g_WinsockManager->SetFileHandle(s, hfile);
    return s;
}
#else
int BindAndListenUnix(const char* path, int backlog, int *err) {
    *err = 0;
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s == -1) {
        *err = tinynet::ERROR_SOCKET_BINDANDLISTEN;
        return -1;
    }
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
    if ((*err = SetNonBlocking(s)) != 0) goto finalize;
    if ((*err = SetReuseAddr(s)) != 0) goto finalize;
    if ((*err = Bind(s, (struct sockaddr*)&addr, sizeof(addr))) != 0) goto finalize;
    if ((*err = Listen(s, backlog) != 0)) goto finalize;
finalize:
    if (*err != 0) {
        ::close(s);
        *err = tinynet::ERROR_SOCKET_BINDANDLISTEN;
        return -1;
    }
    return s;
}
#endif

int SetReuseAddr(int fd) {
#ifndef _WIN32
    int optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == -1) {
        return tinynet::ERROR_SOCKET_REUSEADDR;
    }
#endif
    return 0;
}

int SetReusePort(int fd) {
    int optval = 1;
#ifdef _WIN32
    if (setsockopt((SOCKET)fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) != 0) {
        return tinynet::ERROR_SOCKET_REUSEPORT;
    }
#else
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) != 0) {
        return tinynet::ERROR_SOCKET_REUSEPORT;
    }
#endif
    return 0;
}

int SetNonBlocking(int fd) {
#ifdef _WIN32
    u_long nonblocking = 1;
    if (ioctlsocket((SOCKET)fd, FIONBIO, &nonblocking) == SOCKET_ERROR) {
        return tinynet::ERROR_SOCKET_SETNONBLOCKING;
    }
#else
    int flags;
    if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
        return tinynet::ERROR_SOCKET_SETNONBLOCKING;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return tinynet::ERROR_SOCKET_SETNONBLOCKING;
    }
#endif
    return 0;
}

int Close(int& fd) {
    int res;
#ifdef _WIN32
    res = closesocket((SOCKET)fd);
    g_WinsockManager->ClearSocket(fd);
#else
    res = close(fd);
#endif
    fd = -1;
    return res;
}

static int Accept(int s, struct sockaddr *sa, socklen_t *len) {
    int fd;
#ifdef _WIN32
    fd = tinynet::net::WinsockManager::Accept(s, sa, len);
#else
    fd = accept(s, sa, len);
#endif
    return fd;
}

int Accept(int s, std::string* ip, int* port, int* err) {
    int fd;
    struct sockaddr_storage sa;
    socklen_t len = sizeof(sa);
    fd = -1;
    *err = 0;
    while (true) {
        fd = Accept(s, (struct sockaddr*)&sa, &len);
        if (fd == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            *err = tinynet::ERROR_SOCKET_ACCEPT;
        }
        break;
    }
    if (fd == -1) {
        return -1;
    }
    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        if (ip) {
            ip->resize(INET_ADDRSTRLEN);
            const char* str = inet_ntop(AF_INET, &s->sin_addr, &(*ip)[0], INET_ADDRSTRLEN);
            if (str == NULL) {
                ip->resize(0);
            } else {
                ip->resize(strlen(str));
            }
        }
        if (port) *port = ntohs(s->sin_port);
    } else if(sa.ss_family == AF_INET6) {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        if (ip) {
            ip->resize(INET6_ADDRSTRLEN);
            const char* str = inet_ntop(AF_INET6, &s->sin6_addr, &(*ip)[0], INET6_ADDRSTRLEN);
            if (str == NULL) {
                ip->resize(0);
            } else {
                ip->resize(strlen(str));
            }
        }
        if (port) *port = ntohs(s->sin6_port);
    } else if (sa.ss_family == AF_UNIX) {
#ifdef _WIN32
        if (ip) *ip = "Unknown";
        if (port) *port = 0;
#else
        if (ip) *ip = "Unix domain socket";
        if (port) *port = 0;
        //struct sockaddr_un *s = (struct sockaddr_un *)&sa;
        //if (ip) ip->append(s->sun_path);
        //if (port) *port = 0;
#endif
    } else {
        if (ip) *ip = "Unknown";
        if (port) *port = 0;
    }
    return fd;
}

// Nonblocking socket connect
int Connect(int fd, const struct sockaddr *addr, int addrlen) {
    int err;
    if ((err = SetReuseAddr(fd)) != 0) return err;
    if ((err = SetNonBlocking(fd)) != 0) return err;
    int res = ::connect(fd, addr, addrlen);
#ifdef _WIN32
    if (res == SOCKET_ERROR) {
        int code = WSAGetLastError();
        if (code != WSAEWOULDBLOCK && code != WSAEINPROGRESS) {
            err = tinynet::ERROR_SOCKET_CONNECT;
        }
    }
#else
    if (res == -1) {
        if (errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) {
            err = tinynet::ERROR_SOCKET_CONNECT;
        }
    }
#endif
    return err;
}

int Read(int fd, void *buf, size_t len) {
#ifdef _WIN32
    DWORD bytes, flags;
    WSABUF wsa_buf;
    wsa_buf.buf = (char*)buf;
    wsa_buf.len = static_cast<ULONG>(len);
    flags = 0;
    if (tinynet::net::WinsockManager::WinSock_WSARecv(fd, &wsa_buf, 1, &bytes, &flags, NULL, NULL) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            errno = EAGAIN;
        } else if (err == WSAEINTR) {
            errno = EINTR;
        } else if (err == WSAECONNABORTED) {
            errno = ECONNRESET;
        } else {
            errno = err;
        }
        return -1;
    }
    return bytes;
    /*int ret;
    if ((ret = ::recv((SOCKET)fd, (char*)buf, (int)len, 0)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            errno = EAGAIN;
        } else if (err == WSAEINTR) {
            errno = EINTR;
        } else if (err == WSAECONNABORTED) {
            errno = ECONNRESET;
        } else {
            errno = err;
        }
        return -1;
    }
    return ret;*/
#else
    return read(fd, buf, len);
#endif
}

int Write(int fd, const void *data, size_t len) {
#ifdef _WIN32
    DWORD bytes;
    WSABUF wsa_buf;
    wsa_buf.buf = const_cast<char*>((const char*)data);
    wsa_buf.len = (ULONG)len;
    if (tinynet::net::WinsockManager::WinSock_WSASend(fd, &wsa_buf, 1, &bytes, 0, NULL, NULL) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            errno = EAGAIN;
        } else if (err == WSAEINTR) {
            errno = EINTR;
        } else if (err == WSAECONNABORTED) {
            errno = ECONNRESET;
        } else {
            errno = err;
        }
        return -1;
    }
    return bytes;
    /*int ret;
    if ((ret = ::send((SOCKET)fd, (const char*)data, (int)len, 0)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            errno = EAGAIN;
        } else if (err == WSAEINTR) {
            errno = EINTR;
        } else if (err == WSAECONNABORTED) {
            errno = ECONNRESET;
        } else {
            errno = err;
        }
        return -1;
    }
    return ret;*/
#else
    return write(fd, data, len);
#endif
}

int Readv(int fd, tinynet::iov_t* iov, int len) {
#ifdef _WIN32
    DWORD bytes, flags;
    WSABUF* buf = reinterpret_cast<WSABUF*>(iov);
    flags = 0;
    if (tinynet::net::WinsockManager::WinSock_WSARecv(fd, buf, len, &bytes, &flags, NULL, NULL) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            errno = EAGAIN;
        } else if (err == WSAEINTR) {
            errno = EINTR;
        } else if (err == WSAECONNABORTED) {
            errno = ECONNRESET;
        } else {
            errno = err;
        }
        return -1;
    }
    return bytes;
#else
    return readv(fd, reinterpret_cast<struct iovec*>(iov), len);
#endif
}

int Writev(int fd, tinynet::iov_t* iov, int len) {
#ifdef _WIN32
    DWORD bytes;
    WSABUF* buf = reinterpret_cast<WSABUF*>(iov);
    if (tinynet::net::WinsockManager::WinSock_WSASend(fd, buf, len, &bytes, 0, NULL, NULL) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            errno = EAGAIN;
        } else if (err == WSAEINTR) {
            errno = EINTR;
        } else if (err == WSAECONNABORTED) {
            errno = ECONNRESET;
        } else {
            errno = err;
        }
        return -1;
    }
    return bytes;
#else
    return writev(fd, reinterpret_cast<struct iovec*>(iov), len);
#endif
}


int ReadAll(int fd, tinynet::iovs_t& iovs, int* nread) {
    int err = tinynet::ERROR_OK;
    *nread = 0;
    int n = (int)Readv(fd, &iovs[0], (int)iovs.size());
    if (n > 0) {
        *nread = n;
    } else if (n == 0) {
        err = tinynet::ERROR_SOCKET_READ_EOF; //zero indicates end of file
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            *nread = 0;
        } else {
            err = tinynet::ERROR_SOCKET_READ;
        }
    }
    return err;
}

int ReadAll(int fd, void* buf, size_t len, int* nread) {
    int err = tinynet::ERROR_OK;
    *nread = 0;
    int n = Read(fd, buf, len);
    if (n > 0) {
        *nread = n;
    } else if (n == 0) {
        err = tinynet::ERROR_SOCKET_READ_EOF; //zero indicates end of file
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            *nread = 0;
        } else {
            err = tinynet::ERROR_SOCKET_READ;
        }
    }
    return err;
}


int WriteAll(int fd, tinynet::iovs_t& iovs, int* nwrite) {
    int err = tinynet::ERROR_OK;
    *nwrite = 0;
    int n = (int)Writev(fd, &iovs[0], (int)iovs.size());
    if (n > 0) {
        *nwrite = n;
    } else if(n == 0) {
        *nwrite = 0; //zero indicates nothing was written
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            *nwrite = 0;
        } else {
            err = tinynet::ERROR_SOCKET_WRITE;
        }
    }
    return err;
}

int WriteAll(int fd, const void *data, size_t len, int* nwrite) {
    int err = tinynet::ERROR_OK;
    *nwrite = 0;
    int n = (int)Write(fd, data, len);
    if (n > 0) {
        *nwrite = n;
    } else if (n == 0) {
        *nwrite = 0; //zero indicates nothing was written
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            *nwrite = 0;
        } else {
            err = tinynet::ERROR_SOCKET_WRITE;
        }
    }
    return err;
}

int ConnectTcp(const char* host, int port, int af, int* err) {
    int s, res;
    char service[6];
    struct addrinfo hints, *begin, *end, *it;
    snprintf(service, sizeof(service), "%d", port);
    service[sizeof(service) - 1] = '\0';
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;;
    end = NULL;
    s = -1;
    *err = 0;

    if((res = getaddrinfo(host, service, &hints, &begin)) != 0) {
        *err = tinynet::ERROR_NET_GETADDRINFO;
        return -1;
    }
    *err = tinynet::ERROR_SOCKET_CONNECT;
    for (it = begin; it != end; it = it->ai_next) {
        s = (int)socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (s == -1) {
            continue;
        }
        if ((*err = Connect(s, it->ai_addr, (int)it->ai_addrlen)) != 0) {
            Close(s);
            continue;
        }
        break;
    }
    freeaddrinfo(begin);
    return s;
}

#ifdef _WIN32
int ConnectUnix(const char* path, int* err) {
    int s;
    std::string filename(path);
    std::wstring wfilename;
    StringUtils::convert_utf8_to_utf16(filename, &wfilename);
    s = -1;
    *err = 0;
    HANDLE hfile = CreateFileW(wfilename.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) {
        *err = tinynet::ERROR_UNIX_SOCKET_CONNECT;
        return -1;
    }
    DWORD dwSize = GetFileSize(hfile, NULL);
    if (dwSize == 0 || dwSize >= kMaxUnixPathFileLength) {
        CloseHandle(hfile);
        *err = tinynet::ERROR_UNIX_SOCKET_CONNECT;
        return -1;
    }
    char buf[kMaxUnixPathFileLength] = { 0 };
    if (!ReadFile(hfile, buf, dwSize, &dwSize, NULL)) {
        CloseHandle(hfile);
        *err = tinynet::ERROR_UNIX_SOCKET_CONNECT;
        return -1;
    }
    CloseHandle(hfile);
    std::string ip;
    int port = 0;
    if (!UriUtils::parse_address(buf, &ip, &port)) {
        *err = tinynet::ERROR_UNIX_SOCKET_CONNECT;
        return -1;
    }
    return ConnectTcp(ip.c_str(), port, AF_UNSPEC, err);
}

#else
int ConnectUnix(const char* path, int* err) {
    int s = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (s == -1) {
        *err = tinynet::ERROR_UNIX_SOCKET_CONNECT;
        return -1;
    }
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
    if ((*err = Connect(s, (struct sockaddr*)&addr, sizeof(struct sockaddr_un))) != 0) {
        ::close(s);
        return -1;
    }
    return s;
}
#endif

#ifdef _WIN32
int WaitEvent(int fd, int mask, int timeout_ms) {
    FD_SET read_set, write_set, except_set;
    FD_SET *readfds, *writefds, *exceptfds;
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    readfds = writefds = exceptfds = NULL;
    if (mask & tinynet::net::EVENT_READABLE) {
        readfds = &read_set;
        FD_ZERO(readfds);
        FD_SET(fd, readfds);
    }
    if (mask & tinynet::net::EVENT_WRITABLE) {
        writefds = &write_set;
        FD_ZERO(writefds);
        FD_SET(fd, writefds);
    }
    if (mask & tinynet::net::EVENT_ERROR) {
        exceptfds = &except_set;
        FD_ZERO(exceptfds);
        FD_SET(fd, exceptfds);
    }
    int events = tinynet::net::EVENT_NONE;
    int res = select(fd + 1, readfds, writefds, exceptfds, &timeout);
    if (res < 0) {
        events = mask | tinynet::net::EVENT_ERROR;
    } else if(res == 0) {
    } else if (res > 0) {
        if (readfds && FD_ISSET(fd, readfds)) events |= tinynet::net::EVENT_READABLE;
        if (writefds && FD_ISSET(fd, writefds)) events |= tinynet::net::EVENT_WRITABLE;
        if (exceptfds && FD_ISSET(fd, exceptfds)) events = mask | tinynet::net::EVENT_ERROR;
    }
    return events;
}

#else
int WaitEvent(int fd, int mask, int timeout_ms) {
    struct pollfd pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    if (mask & tinynet::net::EVENT_READABLE) pfd.events |= POLLIN;
    if (mask & tinynet::net::EVENT_WRITABLE) pfd.events |= POLLOUT;
    int events = tinynet::net::EVENT_NONE;
    int res = poll(&pfd, 1, timeout_ms);
    if (res < 0) {
        events = mask | tinynet::net::EVENT_ERROR;
    } else if (res == 0) {
    } else if (res > 0) {
        if (pfd.revents & POLLIN) events |= tinynet::net::EVENT_READABLE;
        if (pfd.revents & POLLOUT) events |= tinynet::net::EVENT_WRITABLE;
        if (pfd.revents & POLLERR) events |= tinynet::net::EVENT_ERROR;
        if (pfd.revents & POLLHUP) events =  mask | tinynet::net::EVENT_WRITABLE;
    }
    return events;
}
#endif

int SetKeepAlive(int fd, int interval_ms) {
    int val = 1;

#ifdef _WIN32
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, sizeof(val)) == -1) {
        return -1;
    }

    struct tcp_keepalive in_options = { 0 }, out_options = { 0 };
    DWORD bytes = 0;
    in_options.onoff = TRUE;
    in_options.keepalivetime = interval_ms;
    val = interval_ms / 10;
    if (val == 0) val = 1;
    in_options.keepaliveinterval = val;
    if (WSAIoctl(fd, SIO_KEEPALIVE_VALS, &in_options, sizeof(in_options),
                 &out_options, sizeof(out_options), &bytes, NULL, NULL) == SOCKET_ERROR) {
        return -1;
    }
#else
    int interval = interval_ms / 1000;
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) == -1) {
        return -1;
    }
    val = interval / 3;
    if (val == 0) val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) == -1) {
        return -1;
    }
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) == -1) {
        return -1;
    }
#endif
    return 0;
}

int SetNodelay(int fd) {
    int val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&val, sizeof(val)) == -1) {
        return -1;
    }
    return 0;
}

#ifdef _WIN32
int GetSockError(int fd) {
    int optval = 0;
    int optlen = sizeof(optval);
    int err = getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen);
    if (err == -1) {
        return WSAGetLastError();
    }
    return optval;
}
#else
int GetSockError(int fd) {
    int optval = 0;
    socklen_t optlen = (socklen_t)sizeof(optval);
    int err = getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
    if (err == -1) {
        return errno;
    }
    return optval;
}
#endif

int GetSockName(int fd, std::string* ip, int* port) {
    struct sockaddr_storage name;
#ifdef _WIN32
    int namelen = sizeof(name);
#else
    socklen_t namelen = (socklen_t)sizeof(name);
#endif
    int err = ::getsockname(fd, (sockaddr*)&name, &namelen);
    if (err == -1) return tinynet::ERROR_SOCKET_GETSOCKNAME;

    if (name.ss_family == AF_INET) {
        char buf[INET_ADDRSTRLEN] = { 0 };
        sockaddr_in *addr = (sockaddr_in*)&name;
        if (ip && inet_ntop(name.ss_family, &addr->sin_addr, buf, sizeof(buf))) ip->append(buf);
        if (port) *port = ntohs(addr->sin_port);
    } else if (name.ss_family == AF_INET6) {
        char buf[INET6_ADDRSTRLEN] = { 0 };
        sockaddr_in6 *addr = (sockaddr_in6*)&name;
        if (ip && inet_ntop(name.ss_family, &addr->sin6_addr, buf, sizeof(buf))) ip->append(buf);
        if (port) *port = ntohs(addr->sin6_port);

    }
    return 0;
}

struct ProtocolReg {
    const char* name;
    unsigned int hash;
    int af;
};

#define PROTOCOL_REG(PROTO, AF) { #PROTO, StringUtils::Hash(#PROTO), AF}

static ProtocolReg well_known_protocols[] = {
    PROTOCOL_REG(tcp, AF_INET),
    PROTOCOL_REG(tcp6, AF_INET6),
    PROTOCOL_REG(udp, AF_INET),
    PROTOCOL_REG(udp6, AF_INET6),
    PROTOCOL_REG(unix, AF_UNIX),
    PROTOCOL_REG(http, AF_INET),
    PROTOCOL_REG(https, AF_INET),
    PROTOCOL_REG(ws, AF_INET),
    PROTOCOL_REG(wss, AF_INET),
    PROTOCOL_REG(TCP, AF_INET),
    PROTOCOL_REG(TCP6, AF_INET6),
    PROTOCOL_REG(UDP, AF_INET),
    PROTOCOL_REG(UDP6, AF_INET6),
    PROTOCOL_REG(UNIX, AF_UNIX),
    PROTOCOL_REG(HTTP, AF_INET),
    PROTOCOL_REG(HTTPS, AF_INET),
    PROTOCOL_REG(WS, AF_INET),
    PROTOCOL_REG(WSS, AF_INET),
    {0, 0, 0}
};

int LookupAddressFamily(const char* protocol) {
    int af = AF_UNSPEC;
    unsigned int hash = StringUtils::Hash(protocol);
    for (ProtocolReg* p = well_known_protocols; p->name; p++) {
        if (p->hash == hash) {
            af = p->af;
            break;
        }
    }
    return af;
}

#ifdef _WIN32
int SocketPair(int family, int type, int protocol, int fd[2]) {
    int listener = -1;
    int connector = -1;
    int acceptor = -1;
    struct sockaddr_in listen_addr;
    struct sockaddr_in connect_addr;
    int size;

    listener = (int)socket(AF_INET, type, 0);
    if (listener == -1)
        return -1;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    listen_addr.sin_port = 0;
    if (bind(listener, (struct sockaddr *) &listen_addr, sizeof(listen_addr)) != 0)
        goto fail;
    if (listen(listener, 1) != 0)
        goto fail;

    connector = (int)socket(AF_INET, type, 0);
    if (connector == -1)
        goto fail;
    size = sizeof(connect_addr);
    if (getsockname(listener, (struct sockaddr *) &connect_addr, &size) == -1)
        goto fail;
    if (connect(connector, (struct sockaddr *) &connect_addr,
                sizeof(connect_addr)) == -1)
        goto fail;

    size = sizeof(listen_addr);
    acceptor = (int)accept(listener, (struct sockaddr *) &listen_addr, &size);
    if (acceptor == -1)
        goto fail;
    Close(listener);
    if (getsockname(connector, (struct sockaddr *) &connect_addr, &size) == -1)
        goto fail;
    if (SetNonBlocking(connector) != 0)
        goto fail;
    if (SetNonBlocking(acceptor) != 0)
        goto fail;
    fd[0] = connector;
    fd[1] = acceptor;
    return 0;

fail:
    if (listener != -1)
        Close(listener);
    if (connector != -1)
        Close(connector);
    if (acceptor != -1)
        Close(acceptor);

    return -1;
}
#else
int SocketPair(int family, int type, int protocol, int fd[2]) {
    return ::socketpair(family, type, protocol, fd);
}
#endif // _WIN32
}
