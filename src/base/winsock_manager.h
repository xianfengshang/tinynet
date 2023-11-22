// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#ifdef _WIN32
#include <stdint.h>
#include <array>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include "allocator.h"
#include "singleton.h"

typedef BOOL(WINAPI *sGetQueuedCompletionStatusEx)
(HANDLE CompletionPort,
 LPOVERLAPPED_ENTRY lpCompletionPortEntries,
 ULONG ulCount,
 PULONG ulNumEntriesRemoved,
 DWORD dwMilliseconds,
 BOOL fAlertable);


typedef int (WSAAPI *sWSAStartup)(
    WORD      wVersionRequested,
    LPWSADATA lpWSAData
);

typedef int (WSAAPI *sWSARecv)(
    SOCKET                             s,
    LPWSABUF                           lpBuffers,
    DWORD                              dwBufferCount,
    LPDWORD                            lpNumberOfBytesRecvd,
    LPDWORD                            lpFlags,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);


typedef int (WSAAPI *sWSASend)(
    SOCKET                             s,
    LPWSABUF                           lpBuffers,
    DWORD                              dwBufferCount,
    LPDWORD                            lpNumberOfBytesSent,
    DWORD                              dwFlags,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

namespace tinynet {
namespace net {
enum req_type_t {
    NONE_REQ,
    READ_REQ,
    READ_REQ2,
    WRITE_REQ,
    WRITE_REQ2,
    CONNECT_REQ,
    ACCEPT_REQ,
    CLOSE_REQ,
    MAX_REQ
};

struct req_t {
    OVERLAPPED overlapped;
    req_type_t type{ NONE_REQ };
};

struct accept_req_t: public req_t {
    SOCKET s;
    char buf[sizeof(struct sockaddr_storage)];
    accept_req_t* next;
};

enum FD_FLAG_TYPE {
    FD_FLAG_ATTACHED			=0x00000100,
    FD_FLAG_READ_QUEUED			=0x00000200,
    FD_FLAG_WRITE_QUEUED		=0x00000400,
    FD_FLAG_LISTENING			=0x00000800,
    FD_FLAG_ACCEPT_PENDING		=0x00001000,
    FD_FLAG_ACCEPT_DONE			=0x00002000,
    FD_FLAG_WAITING_CONNECT		=0x00004000,
    FD_FLAG_CONNECT_PENDING		=0x00008000,
    FD_FLAG_READ_QUEUED2		=0x00010000,
    FD_FLAG_WRITE_QUEUED2		=0x00020000,
    FD_FLAG_CLOSE_PENDING		=0x00040000,
    FD_FLAG_CLOSE_DONE			=0x00080000
};

struct sock_t {
    int fd;
    int flags;
    req_t read_req;
    req_t write_req;
    accept_req_t* accept_req;
    volatile long pending_reqs;
    HANDLE hfile;
    char buf[sizeof(struct sockaddr_storage)];
    //void* data;
};

class WinsockManager :
    public tinynet::Singleton<WinsockManager> {
  public:
    WinsockManager();
    ~WinsockManager();
  public:
    const static int MAX_SOCKETS = 65535;
  public:
    void Init();
    void Shutdown();
    sock_t* GetSocket(int fd);
    int ClearSocket(int fd);
    bool bad_fd(int fd) { return fd <= 0 || fd >= MAX_SOCKETS; }
    int SetFileHandle(int fd, HANDLE hfile);
    int SetSockaddrStorage(int fd, struct sockaddr_storage* addr);
    int SetMask(int fd, int mask);
    int UpdateConnectContext(int fd);
    void QueuedIncomingConnection(int fd, accept_req_t* accept_req);
  public:
    static int Accept(int fd, struct sockaddr *sa, socklen_t *len);
    static int Listen(int fd, int backlog);
    static int PostAccept(HANDLE iocp, int listen_fd);
    static int PostRecv(HANDLE iocp, int fd, char* buf, int len);
    static int PostSend(HANDLE iocp, int fd, char* buf, int len);
    static int PostConnect(HANDLE iocp, int fd);
    static int PostEvent(HANDLE iocp, int fd, int event);
  public:
    static BOOL WinSock_AcceptEx(SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength,
                                 DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped);
    static void WinSock_GetAcceptExSockaddrs(SOCKET s, PVOID lpOutputBuffer, DWORD dwReceiveDataLength,  DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength,
            sockaddr **LocalSockaddr, LPINT LocalSockaddrLength, sockaddr **RemoteSockaddr, LPINT RemoteSockaddrLength );

    static BOOL WinSock_Connectex(SOCKET s, const sockaddr *name, int namelen, PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent,
                                  LPOVERLAPPED lpOverlapped);
    static BOOL WinAPI_GetQueuedCompletionStatusEx(HANDLE CompletionPort, LPOVERLAPPED_ENTRY lpCompletionPortEntries, ULONG ulCount, PULONG ulNumEntriesRemoved,
            DWORD dwMilliseconds, BOOL fAlertable);

    static int WinSock_WSAStartup(
        WORD    wVersionRequested,
        LPWSADATA lpWSAData
    );
    static int  WinSock_WSARecv(
        SOCKET                             s,
        LPWSABUF                           lpBuffers,
        DWORD                              dwBufferCount,
        LPDWORD                            lpNumberOfBytesRecvd,
        LPDWORD                            lpFlags,
        LPWSAOVERLAPPED                    lpOverlapped,
        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );
    static int WinSock_WSASend(
        SOCKET                             s,
        LPWSABUF                           lpBuffers,
        DWORD                              dwBufferCount,
        LPDWORD                            lpNumberOfBytesSent,
        DWORD                              dwFlags,
        LPWSAOVERLAPPED                    lpOverlapped,
        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );
  private:
    static sGetQueuedCompletionStatusEx pGetQueuedCompletionStatusEx;
    static sWSAStartup pWSAStartup;
    static sWSARecv pWSARecv;
    static sWSASend pWSASend;
  public:
    std::array<sock_t*, MAX_SOCKETS> fd_map_;
    CRITICAL_SECTION cs_;
};
}
}

#define g_WinsockManager tinynet::net::WinsockManager::Instance()
#endif
