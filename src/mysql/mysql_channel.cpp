// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "mysql_channel.h"
#include "logging/logging.h"
#include "base/error_code.h"
#include "mysql_codec.h"
#include "base/coding.h"
#include "base/crypto.h"
#include "util/string_utils.h"
#include "base/error_code.h"
#include "mysql_auth.h"
#include "mysql_charset.h"
#include "net/ssl_socket.h"

namespace mysql {


MysqlChannel::MysqlChannel(tinynet::EventLoop* loop):
    net::SocketChannel(loop),
    packet_num_(-1),
    cmd_(protocol::Command::COM_SLEEP),
    query_result_state_(QUERY_RESULT_BEGIN),
    client_capabilities_(0),
    server_capabilities_(0),
    codec_(new (std::nothrow) MysqlCodec()),
    querying_(false),
    connect_timer_(INVALID_TIMER_ID),
    handshake_state_(HS_NONE),
    field_count_(0) {
}

MysqlChannel::~MysqlChannel() {
    if (connect_timer_)
        event_loop_->ClearTimer(connect_timer_);
}

void MysqlChannel::Init(const mysql::Config& config) {
    handshake_state_ = HS_NONE;

    config_ = config;
    net::ChannelOptions opts;
    opts.name = "Mysql";
    opts.host = config.host;
    opts.port = config.port;
    opts.path = config.unix_socket;
    opts.debug = config.debug || config.logConnect;
    opts.timeout = config.connectTimeout;
    opts.use_ssl = config.use_ssl;
    SocketChannel::Init(opts);

    if (config_.debug) {
        json::Value value;
        value << config_;
        log_info("[Mysql] Mysql channel(%lld) init:%s", guid_, json::tojson(value).c_str());
    }
}

void MysqlChannel::BeginQuery() {
    packet_num_ = -1;
    query_result_state_ = QUERY_RESULT_BEGIN;
    cmd_ = protocol::Command::COM_QUERY;
    SendCommand(cmd_, &query_->get_request().sql);
    if (config_.debug) {
        const std::string& sql = query_->get_request().sql;
        log_info("[Mysql] Mysql channel(%lld) sending query to mysql server, sql:%.*s", guid_, (int)sql.length(), &sql[0]);
    }
}

void MysqlChannel::EndQuery() {
    if (!is_querying()) return;

    querying_ = false;
    if (config_.debug) {
        if (query_->get_response().code) {
            log_info("[Mysql] Mysql channel(%lld) end query error, code:%d, msg:%s",
                     guid_, query_->get_response().code, query_->get_response().msg.c_str());
        } else {
            log_info("[Mysql] Mysql channel(%lld) end query ok", guid_, query_->get_response().code);
        }
    }
    Invoke(query_callback_, query_);
    query_callback_ = nullptr;
    query_ = nullptr;
}

void MysqlChannel::BeginAuth() {
    Packet pkt;
    if (config_.password.length() > 0) {
        InputAuthPluginData input{ &config_.password, &auth_data_.plugin_name, &auth_data_.plugin_data, &auth_data_.extra_data };
        OutpuAuthPluginData output{ &pkt.data, nullptr };
        invoke_mysql_auth_plugin(input, output);
    } else {
        pkt.data.append(1, '\0');
    }
    SendPacket(&pkt);
}

void MysqlChannel::EndAuth() {
    handshake_state_ = HS_HANDSHAKED;
    if (is_querying()) {
        BeginQuery();
    }
}

void MysqlChannel::SetError(int err, const std::string& msg) {
    if (query_ && query_->get_response().code == 0) {
        query_->get_response().code = err;
        query_->get_response().msg = msg;
    }
}
void MysqlChannel::OnRead() {
    Packet* pkt;
    while (socket_->is_connected() && (pkt = codec_->Read(socket_)) != nullptr) {
        OnPacket(pkt);
    }

}

void MysqlChannel::OnOpen() {
    connect_timer_ = event_loop_->AddTimer(config_.connectTimeout, 0,
                                           std::bind(&MysqlChannel::Close, this, tinynet::ERROR_MYSQL_HANDSHAKE));
}

void MysqlChannel::OnError(int err) {
    SetError(err, tinynet_strerror(err));
}

void MysqlChannel::OnClose() {
    handshake_state_ = HS_NONE;
    EndQuery();
}

void MysqlChannel::OnPacket(Packet* packet) {
    packet_num_ = packet->seq;
    switch (handshake_state_) {
    case HS_NONE:
        OnInitialHandshake(packet);
        break;
    case HS_HANDSHAKING:
        OnAuthResult(packet);
        break;
    case HS_HANDSHAKED:
        OnCommandResp(packet);
        break;
    default:
        log_warning("[Mysql] Mysql channel(%lld) received packet in a wrong state:%d", guid_, (int)handshake_state_);
        break;
    }
}

void MysqlChannel::OnInitialHandshake(Packet* packet) {
    event_loop_->ClearTimer(connect_timer_);
    if (config_.debug) {
        log_info("[Mysql] Mysql channel(%lld) received inital handshake packet", guid_);
    }
    if (packet->type == PACKET_TYPE_ERR) {
        OnHandshakeError(packet);
        return;
    }
    protocol::InitialHandshake_Packet msg;
    if (!msg.ParseFromString(packet->data)) {
        if (config_.debug) {
            log_info("[Mysql] Mysql channel(%lld) Parse inital handshake packet failed", guid_);
        }
        Close(tinynet::ERROR_MYSQL_READINGPACKET);
        return;
    }
    log_info("[Mysql] Mysql channel(%lld) received server info: protcol_version:%d, server_version:%s, thread_id:%d, charset:%s, auth_plugin_name:%s",
             guid_, (int)msg.protocol_version, msg.server_version.c_str(), msg.thread_id, charset_to_name(msg.character_set), msg.auth_plugin_name.c_str());
    if (msg.protocol_version < protocol::VERSION) {
        Close(tinynet::ERROR_MYSQL_PROTOCOLVERSION);
        return;
    }
    if ( !(msg.capability_flags & protocol::CLIENT_PROTOCOL_41)) {
        Close(tinynet::ERROR_MYSQL_PROTOCOLVERSION);
        return;
    }
    if (msg.capability_flags &protocol::CLIENT_COMPRESS) {
        if (config_.debug) {
            log_info("Server support compressed protocol");
        }
    }
    if (msg.capability_flags &protocol::CLIENT_SESSION_TRACK) {
        if (config_.debug) {
            log_info("Server support session track");
        }
    }
    if (msg.capability_flags & protocol::CLIENT_TRANSACTIONS) {
        if (config_.debug) {
            log_info("Server support transaction");
        }
    }
    bool have_ssl = false;
    if (msg.capability_flags &protocol::CLIENT_SSL) {
        have_ssl = true;
        if (config_.debug) {
            log_info("Server support SSL");
        }
    }
    auth_data_.plugin_name = msg.auth_plugin_name;
    auth_data_.plugin_data = msg.auth_plugin_data;
    server_capabilities_ = msg.capability_flags;
    client_capabilities_ = protocol::CLIENT_CAPABILITIES;
    client_capabilities_ = client_capabilities_&
                           (~(protocol::CLIENT_COMPRESS | protocol::CLIENT_SSL | protocol::CLIENT_PROTOCOL_41) |
                            server_capabilities_);
    //if (config_.compress) client_capabilities_ |= protocol::CLIENT_COMPRESS;

    if (config_.debug) {
        log_info("[Mysql] Mysql channel(%lld) Parse inital packet ok, server version:%s", guid_, msg.server_version.c_str());
    }
    if (have_ssl && config_.use_ssl) {

        client_capabilities_ |= protocol::CLIENT_SSL;
        protocol::SSLRequest pkt;
        pkt.capability_flags = protocol::CLIENT_PROTOCOL_41;
        pkt.capability_flags |= protocol::CLIENT_SSL;
        pkt.capability_flags |= protocol::CLIENT_SSL_VERIFY_SERVER_CERT;

        pkt.max_packet_size = (uint32_t)MAX_PACKET_LENGTH;
        pkt.character_set = charset_to_id(config_.encoding.c_str());
        Packet req;
        pkt.SerializeToString(&req.data);
        SendPacket(&req);
        if (config_.debug) {
            log_info("[Mysql] Mysql channel(%lld) sending ssl request packet", guid_);
        }
        auto sock = get_socket<net::SSLSocket>();
        sock->set_estab_callback(std::bind(&MysqlChannel::SendHandshakeResponse, this));
        sock->Handshake(); //SSL handshake
    } else {
        SendHandshakeResponse();
    }
    handshake_state_ = HS_HANDSHAKING;
}

void MysqlChannel::OnHandshakeError(Packet* packet) {
    protocol::Err_Packet err;
    err.ParseFromString(packet->data, server_capabilities_);
    log_error("[Mysql] mysql channel(%lld) handshake error, error_code:%d, error_message:%s",
              guid_, err.error_code, err.error_message.c_str());
    SetError(err.error_code, err.error_message);
    Close(tinynet::ERROR_MYSQL_HANDSHAKE);
}


void MysqlChannel::OnAuthResult(Packet* packet) {
    if (config_.debug) {
        log_info("[Mysql] Mysql channel(%lld) received auth phase packet, type:%d", guid_, (int)packet->type);
    }
    switch (packet->type) {
    case PACKET_TYPE_OK:
    case PACKET_TYPE_EOF: {
        EndAuth();
        return;
    }
    case PACKET_TYPE_ERR: {
        OnHandshakeError(packet);
        return;
    }
    case PACKET_TYPE_AUTHMOREDATA: {
        protocol::AuthMoreData pkt;
        if (!pkt.ParseFromString(packet->data)) {
            Close(tinynet::ERROR_MYSQL_READINGPACKET);
            return;
        }
        if (pkt.payload.length() > 1) {
            auth_data_.extra_data = pkt.payload;
            BeginAuth();
        } else if (pkt.payload.length() == 1) {
            if (pkt.payload[0] == protocol::FAST_AUTH_SUCCESS) {
                EndAuth();
            } else if (pkt.payload[0] == protocol::PERFORM_FULL_AUTHENTICATION) {
                auth_data_.extra_data.append(1, protocol::PERFORM_FULL_AUTHENTICATION);
                BeginAuth();
            }
        }
        return;
    }
    case PACKET_TYPE_DATA: {
        protocol::AuthSwitchRequest req;
        if (!req.ParseFromString(packet->data)) {
            Close(tinynet::ERROR_MYSQL_READINGPACKET);
            return;
        }
        if (config_.debug) {
            log_info("[Mysql] Mysql channel(%lld) auth switched, auth_plugin_name:%s", guid_, req.auth_method_name.c_str());
        }
        auth_data_.plugin_name = req.auth_method_name;
        auth_data_.plugin_data = req.auth_method_data;
        BeginAuth();
        return;
    }
    default:
        log_info("[Mysql] Mysql channel(%lld) received unknown auth phase packet, type:%d", guid_, (int)packet->type);
        Close(tinynet::ERROR_MYSQL_READINGPACKET);
        break;
    }
}

void MysqlChannel::OnCommandResp(Packet* packet) {
    switch (cmd_) {
    case mysql::protocol::Command::COM_QUERY:
        OnQueryResp(packet);
        break;
    case mysql::protocol::Command::COM_PING:
        OnPingResp(packet);
        break;
    default:
        log_error("Unsuported text protocol:%d", cmd_);
        break;
    }
}

void MysqlChannel::SendPacket(Packet* packet) {
    packet->len = static_cast<uint32_t>(packet->data.length());
    packet->seq = ++packet_num_;
    codec_->Write(socket_, packet);
}

void MysqlChannel::SendHandshakeResponse() {
    protocol::HandshakeResponse_Packet pkt;
    pkt.max_packet_size = (uint32_t)MAX_PACKET_LENGTH;
    pkt.character_set = charset_to_id(config_.encoding.c_str());
    pkt.username = config_.user;
    pkt.auth_plugin_name = auth_data_.plugin_name;
    if (config_.password.length() > 0) {
        InputAuthPluginData input{ &config_.password, &auth_data_.plugin_name, &auth_data_.plugin_data, &auth_data_.extra_data };
        OutpuAuthPluginData output{ &pkt.auth_response, &client_capabilities_ };
        invoke_mysql_auth_plugin(input, output);
    }
    pkt.database = config_.database;
    pkt.capability_flags = client_capabilities_;

    Packet req;
    pkt.SerializeToString(&req.data);
    SendPacket(&req);
    client_capabilities_ |= pkt.capability_flags;
}

void MysqlChannel::SendCommand(protocol::Command cmd, const std::string* data) {
    Packet pkt;
    pkt.data.append(1, (char)cmd);
    if (data == nullptr) {
        SendPacket(&pkt);
        return;
    }
    if (data->length() < (MAX_PACKET_LENGTH - 1) ) {
        pkt.data.append(*data);
        SendPacket(&pkt);
        return;
    }
    size_t pos = 0;
    size_t block_size = MAX_PACKET_LENGTH - 1;
    for (; pos <= data->length(); pos += block_size) {
        pkt.data.clear();
        if (pos == 0) {
            pkt.data.append(1, (char)cmd);
        } else {
            block_size = MAX_PACKET_LENGTH;
        }
        pkt.data.append(*data, pos, block_size); //pos equal data's length will be ok
        SendPacket(&pkt);
    }
}

void MysqlChannel::Ping() {
    if (is_querying())
        return;
    packet_num_ = -1;
    cmd_ = protocol::Command::COM_PING;
    SendCommand(cmd_, nullptr);
}

int MysqlChannel::Query(MysqlQueryPtr query, QueryCallback callback) {
    int err = 0;
    if (is_querying()) {
        return tinynet::ERROR_MYSQL_QUERYBUSY;
    }
    querying_ = true;
    query_ = query;
    query_callback_ = std::move(callback);

    switch (get_state()) {
    case net::ChannelState::CS_UNSPEC:
    case net::ChannelState::CS_CLOSED:
        err = Open();
        break;
    case net::ChannelState::CS_CONNECTING:
        break;
    case net::ChannelState::CS_CONNECTED: {
        if (handshake_state_ == HS_HANDSHAKED) {
            BeginQuery();
        }
        break;
    }
    default:
        err = ERROR_FAILED;
        break;
    }
    if (err != ERROR_OK) {
        querying_ = false;
        query_callback_ = nullptr;
    }
    return err;
}

void MysqlChannel::OnPingResp(Packet* packet) {
    if (packet->type == PACKET_TYPE_ERR) {
        protocol::Err_Packet err;
        err.ParseFromString(packet->data, server_capabilities_);
        log_error("[Mysql] mysql channel(%lld) ping err, error_code:%d, error_message:%s",
                  guid_, err.error_code, err.error_message.c_str());
        return;
    }
    if (packet->type != PACKET_TYPE_OK) {
        log_error("[Mysql] mysql channel(%lld) ping to mysql server failed", guid_);
        return;
    }
}


void MysqlChannel::ParseResultSetBegin(Packet* packet) {
    if (config_.debug) {
        log_info("[Mysql] mysql channel(%lld) parse result set begin", guid_);
    }
    if (packet->type == PACKET_TYPE_ERR) {
        protocol::Err_Packet err;
        err.ParseFromString(packet->data, server_capabilities_);
        query_->get_response().code = err.error_code;
        query_->get_response().msg = err.error_message;
        query_result_state_ = QUERY_RESULT_END;
        return;
    }
    query_->get_response().results.emplace_back(QueryResult());
    if (packet->type == PACKET_TYPE_OK) {
        protocol::OK_Packet ok;
        if (!ok.ParseFromString(packet->data, server_capabilities_)) {
            Close(tinynet::ERROR_MYSQL_READINGPACKET);
            return;
        }
        query_->get_response().results.back().affectedRows = ok.affected_rows;
        query_->get_response().results.back().insertId = ok.last_insert_id;
        query_result_state_ = QUERY_RESULT_END;
        return;
    } else if (packet->type == PACKET_TYPE_EOF) {
        protocol::EOF_Packet eof;
        if (!eof.ParseFromString(packet->data, server_capabilities_)) {
            Close(tinynet::ERROR_MYSQL_READINGPACKET);
            return;
        }
        query_result_state_ = QUERY_RESULT_END;
        return;
    }
    size_t pos = 0;
    protocol::LengthEncodedInteger filed_count;
    filed_count.ParseFromString(packet->data, pos);
    query_result_state_ = QUERY_RESULT_COLUMN;
    field_count_ = (size_t)filed_count.value;
}

void MysqlChannel::ParseDataColumn(Packet* packet) {
    if (packet->type == PACKET_TYPE_ERR) {
        protocol::Err_Packet err;
        err.ParseFromString(packet->data, server_capabilities_);
        query_->get_response().code = err.error_code;
        query_->get_response().msg = err.error_message;
        query_result_state_ = QUERY_RESULT_END;
        return;
    }
    if (packet->type == PACKET_TYPE_OK) {
        protocol::OK_Packet ok;
        ok.ParseFromString(packet->data, server_capabilities_);
        query_result_state_ = QUERY_RESULT_ROW;
        return;
    } else if (packet->type == PACKET_TYPE_EOF) {
        protocol::EOF_Packet eof;
        eof.ParseFromString(packet->data, server_capabilities_);
        query_result_state_ = QUERY_RESULT_ROW;
        return;
    }
    size_t pos = 0;
    protocol::ColumnDefinition column;
    if (!column.ParseFromString(packet->data, pos)) {
        Close(tinynet::ERROR_MYSQL_READINGPACKET);
        return;
    }
    DataColumn data_column;
    data_column.name = column.name;
    data_column.type = column.type;
    query_->get_response().results.back().fields.emplace_back(std::move(data_column));
    if ((server_capabilities_  & protocol::CLIENT_DEPRECATE_EOF) &&
            query_->get_response().results.back().fields.size() == field_count_) {
        query_result_state_ = QUERY_RESULT_ROW;
        return;
    }
}

void MysqlChannel::ParseDataRow(Packet* packet) {
    if (packet->type == PACKET_TYPE_ERR) {
        protocol::Err_Packet err;
        err.ParseFromString(packet->data, server_capabilities_);
        query_->get_response().code = err.error_code;
        query_->get_response().msg = err.error_message;
        query_result_state_ = QUERY_RESULT_END;
        return;
    }
    if (packet->type == PACKET_TYPE_OK) {
        protocol::OK_Packet ok;
        ok.ParseFromString(packet->data, server_capabilities_);
        if (ok.status_flags & (int)protocol::StatusFlags::SERVER_MORE_RESULTS_EXISTS) {
            query_result_state_ = QUERY_RESULT_BEGIN;
        } else {
            query_result_state_ = QUERY_RESULT_END;
        }
        return;
    } else if (packet->type == PACKET_TYPE_EOF) {
        protocol::EOF_Packet eof;
        eof.ParseFromString(packet->data, server_capabilities_);
        if (eof.status_flags & (int)protocol::StatusFlags::SERVER_MORE_RESULTS_EXISTS) {
            query_result_state_ = QUERY_RESULT_BEGIN;
        } else {
            query_result_state_ = QUERY_RESULT_END;
        }
        return;
    }
    QueryResult& result = query_->get_response().results.back();
    DataRow data_row;
    data_row.resize(result.fields.size());
    size_t pos = 0;
    for (size_t i = 0; i < result.fields.size(); ++i) {
        protocol::LengthEncodedString value(data_row[i]);
        value.ParseFromString(packet->data, pos, protocol::NullSafety::Nullable);

    }
    result.rows.emplace_back(std::move(data_row));
}

void MysqlChannel::ParseResultSetEnd() {
    if (config_.debug) {
        log_info("[Mysql] mysql channel(%lld) parse result set end", guid_);
    }
    EndQuery();
}

void MysqlChannel::OnQueryResp(Packet* packet) {
    switch (query_result_state_) {
    case QUERY_RESULT_BEGIN:
        ParseResultSetBegin(packet);
        break;
    case QUERY_RESULT_COLUMN:
        ParseDataColumn(packet);
        break;
    case QUERY_RESULT_ROW:
        ParseDataRow(packet);
        break;
    default:
        break;
    }
    if (query_result_state_ == QUERY_RESULT_END) {
        ParseResultSetEnd();
    }
}

}
