// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "net/socket_channel.h"
#include "net/event_loop.h"
#include "mysql_types.h"
#include "mysql_protocol.h"
#include "mysql_query.h"

using namespace tinynet;

namespace mysql {

enum ChannelState {
    STATE_UNSPEC = 0,
    STATE_CONNECTING = 1,
    STATE_HANDSHAKE = 2,
    STATE_CONNECTED = 3,
    STATE_CLOSED = 4,
};

enum QueryResultState {
    QUERY_RESULT_BEGIN = 0,
    QUERY_RESULT_HEADER = 1,
    QUERY_RESULT_COLUMN = 2,
    QUERY_RESULT_ROW = 3,
    QUERY_RESULT_END = 4
};

class MysqlCodec;

class MysqlChannel :
    public net::SocketChannel {
  public:
    typedef std::function<void(MysqlQueryPtr)> QueryCallback;
    enum HandshakeState {
        HS_NONE,
        HS_HANDSHAKING,
        HS_HANDSHAKED
    };
  public:
    MysqlChannel(tinynet::EventLoop* loop);
    ~MysqlChannel();
  public:
    void Init(const mysql::Config& config);
    int Query(MysqlQueryPtr query, QueryCallback callback);
  private:
    void OnRead() override;
    void OnOpen() override;
    void OnError(int err) override;
    void OnClose() override;
  private:
    void Ping();
    void BeginQuery();
    void EndQuery();
    void BeginAuth();
    void EndAuth();
    void SetError(int err, const std::string& msg);
    void SendPacket(Packet* packet);
    void SendCommand(protocol::Command cmd, const std::string* data);
    void SendHandshakeResponse();
    void OnPacket(Packet* packet);
    void OnInitialHandshake(Packet* packet);
    void OnHandshakeError(Packet* packet);
    void OnAuthResult(Packet* pakcet);
    void OnCommandResp(Packet* packet);
    void OnPingResp(Packet* packet);
    void OnQueryResp(Packet* packet);

    void ParseResultSetBegin(Packet* packet);
    void ParseDataColumn(Packet* packet);
    void ParseDataRow(Packet* packet);
    void ParseResultSetEnd();
  public:
    bool is_querying() const { return querying_; }
  private:
    struct AuthPhaseData {
        std::string plugin_name;
        std::string plugin_data;
        std::string extra_data;
    };
  private:
    int               packet_num_;
    protocol::Command cmd_;
    QueryResultState  query_result_state_;
    uint32_t          client_capabilities_;
    uint32_t		  server_capabilities_;
    std::unique_ptr<MysqlCodec> codec_;
    MysqlQueryPtr     query_;
    QueryCallback     query_callback_;
    Config            config_;
    bool              querying_;
    AuthPhaseData     auth_data_;
    int64_t           connect_timer_;
    HandshakeState    handshake_state_;
    size_t			  field_count_;
};
}