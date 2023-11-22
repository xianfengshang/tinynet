// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifdef _WIN32
#include "winsock_manager.h"
#include <WinSock2.h>
#include "runtime_logger.h"

namespace tinynet {
namespace net {

static long kMaxPendingAcceptReqs = 1;
static DWORD kAddressLength = 64;

static void sock_init(sock_t* sock, int fd) {
    sock->fd = fd;
    sock->flags = 0;
    sock->hfile = INVALID_HANDLE_VALUE;
    sock->accept_req = NULL;
    sock->pending_reqs = 0;
    sock->read_req.type = NONE_REQ;
    memset(&sock->read_req.overlapped, 0, sizeof(sock->read_req.overlapped));
    sock->write_req.type = NONE_REQ;
    memset(&sock->write_req.overlapped, 0, sizeof(sock->write_req.overlapped));
    memset(sock->buf, 0, sizeof(sock->buf));
}

static sock_t* sock_new(int fd) {
    sock_t* sock = (sock_t*)malloc(sizeof(sock_t));
    sock_init(sock, fd);
    return sock;
}

static void sock_delete(sock_t* sock) {
    if (sock == NULL) return;
    sock->flags = 0;
    if (sock->hfile != INVALID_HANDLE_VALUE) {
        CloseHandle(sock->hfile);
        sock->hfile = INVALID_HANDLE_VALUE;
    }
    if (sock->accept_req != NULL) {
        accept_req_t *head, *it;
        for (head = sock->accept_req; head != NULL;) {
            it = head;
            head = it->next;
            closesocket(it->s);
            free(it);
        }
        sock->accept_req = NULL;
    }
    free(sock);
}

static accept_req_t* accept_req_new() {
    accept_req_t* accept_req = (accept_req_t*)malloc(sizeof(accept_req_t));
    if (!accept_req) return NULL;
    ZeroMemory(&accept_req->overlapped, sizeof(accept_req->overlapped));
    accept_req->s = INVALID_SOCKET;
    accept_req->next = NULL;
    accept_req->type = ACCEPT_REQ;
    return accept_req;
}

static void accept_req_delete(accept_req_t* accept_req) {
    if (accept_req->s != INVALID_SOCKET) {
        closesocket(accept_req->s);
        accept_req->s = INVALID_SOCKET;
    }
    free(accept_req);
}

sGetQueuedCompletionStatusEx WinsockManager::pGetQueuedCompletionStatusEx = NULL;

sWSAStartup WinsockManager::pWSAStartup = NULL;
sWSASend WinsockManager::pWSASend = NULL;
sWSARecv WinsockManager::pWSARecv = NULL;

WinsockManager::WinsockManager():
    fd_map_() {
    InitializeCriticalSection(&cs_);
}

WinsockManager::~WinsockManager() {
    DeleteCriticalSection(&cs_);
}

void WinsockManager::Init() {
    WSADATA data;
    if (WinSock_WSAStartup(MAKEWORD(2, 2), &data) != 0 || LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 2) {
        log_runtime_error("Initialize winsock failed");
        exit(1);
    }
    HMODULE kernel32_module = GetModuleHandleA("kernel32.dll");
    if (kernel32_module == NULL) {
        log_runtime_error("GetModuleHandleA(\"kernel32\") failed");
        exit(1);
    }
    pGetQueuedCompletionStatusEx = (sGetQueuedCompletionStatusEx)GetProcAddress(
                                       kernel32_module,
                                       "GetQueuedCompletionStatusEx");

    for (int i = 0; i < MAX_SOCKETS; ++i) {
        fd_map_[i] = NULL;
    }
}


void WinsockManager::Shutdown() {
    for (int i = 0; i < MAX_SOCKETS; ++i) {
        sock_delete(fd_map_[i]);
        fd_map_[i] = NULL;
    }
    WSACleanup();
}

sock_t* WinsockManager::GetSocket(int fd) {
    if (bad_fd(fd)) return NULL;
    sock_t** sock;
    EnterCriticalSection(&cs_);
    sock = &fd_map_[fd];
    if (*sock == NULL) {
        *sock = sock_new(fd);
    }
    LeaveCriticalSection(&cs_);
    return *sock;
}

int WinsockManager::ClearSocket(int fd) {
    if (bad_fd(fd)) return -1;
    sock_t** sock;
    EnterCriticalSection(&cs_);
    sock = &fd_map_[fd];
    if (*sock != NULL) {
        sock_delete(*sock);
        *sock = NULL;
    }
    LeaveCriticalSection(&cs_);
    return 0;
}


int WinsockManager::SetFileHandle(int fd, HANDLE hfile) {
    sock_t* sock = GetSocket(fd);
    if (!sock) return -1;
    if (sock->hfile != INVALID_HANDLE_VALUE) {
        CloseHandle(sock->hfile);
    }
    sock->hfile = hfile;
    return 0;
}

int WinsockManager::SetSockaddrStorage(int fd, struct sockaddr_storage* addr) {
    sock_t* sock = GetSocket(fd);
    if (!sock) return -1;
    if (addr != NULL) {
        memcpy(&sock->buf, addr, sizeof(struct sockaddr_storage));
    }
    return 0;
}

int WinsockManager::SetMask(int fd, int mask) {
    sock_t* sock = GetSocket(fd);
    if (!sock) return -1;
    sock->flags |= mask;
    return 0;
}

int WinsockManager::UpdateConnectContext(int fd) {
    return setsockopt((SOCKET)fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
}

void WinsockManager::QueuedIncomingConnection(int fd, accept_req_t* accept_req) {
    sock_t* sock = g_WinsockManager->GetSocket(fd);
    if (sock == NULL) {
        if (accept_req) {
            closesocket(accept_req->s);
            free(accept_req);
        }
        return;
    }
    InterlockedDecrement(&sock->pending_reqs);
    accept_req_t* head = sock->accept_req;
    accept_req->next = head;
    sock->accept_req = accept_req;

}

int WinsockManager::Accept(int fd, struct sockaddr *sa, socklen_t *len) {
    sock_t* sock = g_WinsockManager->GetSocket(fd);
    if (sock == NULL) {
        errno = EINVAL;
        return SOCKET_ERROR;
    }
    if ((sock->flags & FD_FLAG_LISTENING) == 0) {
        errno = EINVAL;
        return SOCKET_ERROR;
    }
    SOCKET sAcceptSocket;
    if ((sock->flags & FD_FLAG_ATTACHED) == 0) {
        sAcceptSocket = accept((SOCKET)fd, sa, len);
        if (sAcceptSocket == INVALID_SOCKET) {
            int err = WSAGetLastError();
            if (err == WSAEINTR) {
                errno = EINTR;
            } else if (err == WSAEINVAL) {
                errno = EINVAL;
            } else if (err == WSAEWOULDBLOCK) {
                errno = EWOULDBLOCK;
            } else {
                errno = EINVAL;
            }
        }
        return (int)sAcceptSocket;
    }
    accept_req_t* head = sock->accept_req;
    if (head == NULL) {
        errno = EAGAIN;
        return SOCKET_ERROR;
    }
    sock->accept_req = head->next;
    sAcceptSocket = head->s;
    head->s = INVALID_SOCKET;
    if (sAcceptSocket == INVALID_SOCKET) {
        accept_req_delete(head);
        errno = EBADF;
        return SOCKET_ERROR;
    }
    int res = setsockopt(sAcceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&sAcceptSocket, sizeof(sAcceptSocket));
    if (res == SOCKET_ERROR) {
        accept_req_delete(head);
        errno = EAGAIN;
        return SOCKET_ERROR;
    }
    SOCKADDR *pLocalAddr = NULL;
    SOCKADDR *pRemoteAddr = NULL;
    int localLen = 0;
    int remoteLen = 0;
    INET_ADDRSTRLEN;
    WinSock_GetAcceptExSockaddrs(sAcceptSocket, head->buf, 0, kAddressLength, kAddressLength, &pLocalAddr, &localLen, &pRemoteAddr, &remoteLen);
    if (sa != NULL) {
        if (remoteLen > 0) {
            if (remoteLen < *len) {
                *len = remoteLen;
            }
            memcpy(sa, pRemoteAddr, *len);
        } else {
            *len = 0;
        }
    }
    accept_req_delete(head);
    return (int)sAcceptSocket;
}

int WinsockManager::Listen(int fd, int backlog) {
    sock_t* sock = g_WinsockManager->GetSocket(fd);
    if (!sock) {
        errno = EINVAL;
        return SOCKET_ERROR;
    }
    sock->flags |= FD_FLAG_LISTENING;
    sock->pending_reqs = 0;
    if (listen(fd, backlog) != 0) {
        return SOCKET_ERROR;
    }
    return 0;
}

int WinsockManager::PostAccept(HANDLE iocp, int listen_fd) {
    DWORD bytes;
    sock_t* listen_sock = g_WinsockManager->GetSocket(listen_fd);
    if (!listen_sock) {
        errno = WSAEINVAL;
        return SOCKET_ERROR;
    }
    if (listen_sock->pending_reqs >= kMaxPendingAcceptReqs) {
        return 0;
    }
    for (int i = 0; i < kMaxPendingAcceptReqs; ++i) {
        SOCKET sAcceptSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sAcceptSocket == INVALID_SOCKET) {
            errno = ENFILE;
            return SOCKET_ERROR;
        }
        sock_t* accept_sock = g_WinsockManager->GetSocket((int)sAcceptSocket);
        if (!accept_sock) {
            closesocket(sAcceptSocket);
            errno = EINVAL;
            return SOCKET_ERROR;
        }
        accept_req_t* accept_req = accept_req_new();
        if (!accept_req) {
            errno = ENOMEM;
            return SOCKET_ERROR;
        }
        accept_req->s = sAcceptSocket;
        BOOL res = WinSock_AcceptEx((SOCKET)listen_fd, sAcceptSocket, accept_req->buf, 0, kAddressLength, kAddressLength, &bytes, &accept_req->overlapped);
        if (res == FALSE && WSAGetLastError() != WSA_IO_PENDING) {
            accept_req_delete(accept_req);
            errno = ECONNABORTED;
            return SOCKET_ERROR;
        }
        if (InterlockedIncrement(&listen_sock->pending_reqs) >= kMaxPendingAcceptReqs) {
            break;
        }
    }
    return 0;
}

int WinsockManager::PostRecv(HANDLE iocp, int fd, char* buf, int len) {
    sock_t* sock = g_WinsockManager->GetSocket(fd);
    if (!sock) {
        errno = WSAEINVAL;
        return SOCKET_ERROR;
    }
    WSABUF wsa_buf;
    wsa_buf.buf = buf == NULL && len == 0 ? &sock->buf[0] : buf;
    wsa_buf.len = len;
    DWORD byte_received = 0;
    DWORD recv_flags = 0;
    ZeroMemory(&sock->read_req.overlapped, sizeof(sock->read_req.overlapped));
    sock->read_req.type = READ_REQ;
    int res = WinSock_WSARecv(fd, &wsa_buf, 1, &byte_received, &recv_flags, &sock->read_req.overlapped, NULL);
    if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        errno = WSAGetLastError();
        sock->read_req.overlapped.Internal = STATUS_INVALID_HANDLE;
        if (PostQueuedCompletionStatus(iocp, 0, fd, &sock->read_req.overlapped) == FALSE && GetLastError()!= WSA_IO_PENDING) {
            errno = GetLastError();
            sock->flags &= ~FD_FLAG_READ_QUEUED;
            return SOCKET_ERROR;
        }
    }
    sock->flags |= FD_FLAG_READ_QUEUED;
    return 0;
}

int WinsockManager::PostSend(HANDLE iocp, int fd, char* buf, int len) {
    sock_t* sock = g_WinsockManager->GetSocket(fd);
    if (!sock) {
        errno = WSAEINVAL;
        return SOCKET_ERROR;
    }
    WSABUF wsa_buf;
    wsa_buf.buf = buf == NULL && len == 0 ? &sock->buf[1] : buf;
    wsa_buf.len = len;
    DWORD bytes_sent = 0;
    ZeroMemory(&sock->write_req.overlapped, sizeof(sock->write_req.overlapped));
    sock->write_req.type = WRITE_REQ;
    int res = WinSock_WSASend(fd, &wsa_buf, 1, &bytes_sent, 0, &sock->write_req.overlapped, 0);
    if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        int err = WSAGetLastError();
        if (err == WSAENOTCONN) {
            return PostConnect(iocp, fd);
        } else {
            sock->write_req.overlapped.Internal = STATUS_INVALID_HANDLE;
            if (PostQueuedCompletionStatus(iocp, 0, fd, &sock->write_req.overlapped) == FALSE && GetLastError() != WSA_IO_PENDING) {
                errno = GetLastError();
                sock->flags &= ~FD_FLAG_WRITE_QUEUED;
                return SOCKET_ERROR;
            }
        }
    }
    sock->flags |= FD_FLAG_WRITE_QUEUED;
    return 0;
}

int WinsockManager::PostConnect(HANDLE iocp, int fd) {
    sock_t* sock = g_WinsockManager->GetSocket(fd);
    if (!sock) {
        errno = EINVAL;
        return SOCKET_ERROR;
    }
    struct sockaddr_storage* peer_addr = reinterpret_cast<struct sockaddr_storage*>(sock->buf);
    int peer_len;

    switch (peer_addr->ss_family) {
    case AF_INET: {
        peer_len = sizeof(SOCKADDR_IN);
        break;
    }
    case AF_INET6: {
        peer_len = sizeof(SOCKADDR_IN6);
        break;
    }
    default: {
        errno = EINVAL;
        return SOCKET_ERROR;
    }
    }
    struct sockaddr_in addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;

    if (bind(fd, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEINVAL) {  //WSAEINVAL: This error is returned of the socket s is already bound to an address
            errno = err;
            return SOCKET_ERROR;
        }
    }
    ZeroMemory(&sock->read_req.overlapped, sizeof(sock->read_req.overlapped));
    sock->read_req.type = CONNECT_REQ;
    int res = WinSock_Connectex(fd, (SOCKADDR*)peer_addr, peer_len, NULL, 0, NULL, &sock->read_req.overlapped);
    if (res != TRUE) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            errno = err;
            sock->read_req.overlapped.Internal = STATUS_INVALID_HANDLE;
            if (PostQueuedCompletionStatus(iocp, 0, fd, &sock->read_req.overlapped) == FALSE && GetLastError() != WSA_IO_PENDING) {
                errno = GetLastError();
                sock->flags &= ~FD_FLAG_CONNECT_PENDING;
                return SOCKET_ERROR;
            }
        }
    }
    sock->flags &= ~FD_FLAG_WAITING_CONNECT;
    sock->flags |= FD_FLAG_CONNECT_PENDING;
    return 0;
}

int WinsockManager::PostEvent(HANDLE iocp, int fd, int event) {
    sock_t* sock = g_WinsockManager->GetSocket(fd);
    if (!sock) {
        errno = EINVAL;
        return SOCKET_ERROR;
    }
    int result = 0;
    switch (event) {
    case READ_REQ2: {
        ZeroMemory(&sock->read_req.overlapped, sizeof(sock->read_req.overlapped));
        sock->read_req.type = READ_REQ2;
        BOOL res = PostQueuedCompletionStatus(iocp, 0, fd, &sock->read_req.overlapped);
        if (res != TRUE && GetLastError() != WSA_IO_PENDING) {
            result = SOCKET_ERROR;
            break;
        }
        sock->flags |= FD_FLAG_READ_QUEUED2;
        break;
    }
    case WRITE_REQ2: {
        ZeroMemory(&sock->write_req.overlapped, sizeof(sock->write_req.overlapped));
        sock->write_req.type = WRITE_REQ2;
        BOOL res = PostQueuedCompletionStatus(iocp, 0, fd, &sock->write_req.overlapped);
        if (res != TRUE && GetLastError() != WSA_IO_PENDING) {
            result =  SOCKET_ERROR;
            break;
        }
        sock->flags |= FD_FLAG_WRITE_QUEUED2;
        break;
    }
    default:
        errno = EINVAL;
        result =  SOCKET_ERROR;
        break;
    }
    return result;
}

BOOL WinsockManager::WinSock_AcceptEx(SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength,
                                      DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped) {
    LPFN_ACCEPTEX acceptex;
    const GUID wsaid_acceptex = WSAID_ACCEPTEX;
    DWORD bytes;

    if (SOCKET_ERROR == WSAIoctl(sListenSocket,SIO_GET_EXTENSION_FUNCTION_POINTER, (void *)&wsaid_acceptex, sizeof(GUID),
                                 &acceptex, sizeof(LPFN_ACCEPTEX), &bytes, NULL,NULL)) {
        return FALSE;
    }
    return acceptex(sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped);
}

void WinsockManager::WinSock_GetAcceptExSockaddrs(SOCKET s, PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength,
        sockaddr **LocalSockaddr, LPINT LocalSockaddrLength, sockaddr **RemoteSockaddr, LPINT RemoteSockaddrLength) {
    LPFN_GETACCEPTEXSOCKADDRS getacceptsockaddrs;
    const GUID wsaid_getacceptsockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
    DWORD bytes;

    if (SOCKET_ERROR == WSAIoctl(s,
                                 SIO_GET_EXTENSION_FUNCTION_POINTER,
                                 (void *)&wsaid_getacceptsockaddrs,
                                 sizeof(GUID),
                                 &getacceptsockaddrs,
                                 sizeof(LPFN_ACCEPTEX),
                                 &bytes,
                                 NULL,
                                 NULL)) {
        return;
    }

    getacceptsockaddrs(lpOutputBuffer,
                       dwReceiveDataLength,
                       dwLocalAddressLength,
                       dwRemoteAddressLength,
                       LocalSockaddr,
                       LocalSockaddrLength,
                       RemoteSockaddr,
                       RemoteSockaddrLength);
}

BOOL WinsockManager::WinSock_Connectex(SOCKET s, const sockaddr *name, int namelen, PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent,
                                       LPOVERLAPPED lpOverlapped) {
    LPFN_CONNECTEX connectex;
    const GUID wsaid_connectex = WSAID_CONNECTEX;
    DWORD bytes;

    if (SOCKET_ERROR == WSAIoctl(s,SIO_GET_EXTENSION_FUNCTION_POINTER, (void *)&wsaid_connectex, sizeof(GUID), &connectex,  sizeof(LPFN_ACCEPTEX), &bytes, NULL,NULL)) {
        return FALSE;
    }
    return connectex(s, name, namelen,lpSendBuffer,dwSendDataLength,lpdwBytesSent,lpOverlapped);
}

BOOL WinsockManager::WinAPI_GetQueuedCompletionStatusEx(HANDLE CompletionPort, LPOVERLAPPED_ENTRY lpCompletionPortEntries, ULONG ulCount, PULONG ulNumEntriesRemoved, DWORD dwMilliseconds, BOOL fAlertable) {
    if (pGetQueuedCompletionStatusEx == NULL) {
        return FALSE;
    }
    return pGetQueuedCompletionStatusEx(CompletionPort, lpCompletionPortEntries, ulCount, ulNumEntriesRemoved, dwMilliseconds, fAlertable);
}

int WinsockManager::WinSock_WSAStartup( WORD    wVersionRequested, LPWSADATA lpWSAData ) {
    if (pWSAStartup == NULL) {
        HMODULE ws_module = GetModuleHandleA("ws2_32.dll");
        if (ws_module == NULL) {
            ws_module = LoadLibraryA("ws2_32.dll");
            if (ws_module == NULL) {
                log_runtime_error("WinSock_WSAStartup failed, can not found module: ws2_32.dll");
                return WSAVERNOTSUPPORTED;
            }
        }
        pWSAStartup = (sWSAStartup)GetProcAddress(ws_module, "WSAStartup");
        if (pWSAStartup == NULL) {
            log_runtime_error("WinSock_WSAStartup failed, can not found api: WSAStartup");
            return WSAVERNOTSUPPORTED;
        }
        pWSARecv = (sWSARecv)GetProcAddress(ws_module, "WSARecv");
        if (pWSARecv == NULL) {
            log_runtime_error("WinSock_WSAStartup failed, can not found api: pWSARecv");
            return WSAVERNOTSUPPORTED;
        }
        SIZE_T dwNum = 0;
        BYTE buf[5] = { 0 };
        if (ReadProcessMemory(GetCurrentProcess(), pWSARecv, buf, 5, &dwNum)) {
            if (buf[0] == 0xe9) { //WSARecv has been hooked by somebody else
                BYTE code[5] = {0x48, 0x89, 0x5c, 0x24, 0x08};
                WriteProcessMemory(GetCurrentProcess(), pWSARecv, code, 5, &dwNum);
            }
        }


        pWSASend = (sWSASend)GetProcAddress(ws_module, "WSASend");
        if (pWSASend == NULL) {
            log_runtime_error("WinSock_WSAStartup failed, can not found api: WSASend");
            return WSAVERNOTSUPPORTED;
        }

        if (ReadProcessMemory(GetCurrentProcess(), pWSASend, buf, 5, &dwNum)) {
            if (buf[0] == 0xe9) { //WSASend has been hooked by somebody else
                BYTE code[5] = { 0x48, 0x89, 0x5c, 0x24, 0x08 };
                WriteProcessMemory(GetCurrentProcess(), pWSASend, code, 5, &dwNum);
            }
        }
    }


    if (pWSAStartup == NULL) {
        return WSAVERNOTSUPPORTED;
    }
    return pWSAStartup(wVersionRequested, lpWSAData);
}

int  WinsockManager::WinSock_WSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd,
                                     LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    if (pWSARecv == NULL) {
        WSASetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }
    return pWSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
}

int WinsockManager::WinSock_WSASend(SOCKET s, LPWSABUF  lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags,
                                    LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine ) {
    if (pWSASend == NULL) {
        WSASetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }
    return pWSASend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
}

}
}
#endif
