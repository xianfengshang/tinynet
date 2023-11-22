// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <vector>
namespace mysql {

const size_t MAX_PACKET_LENGTH = 0xffffff;

enum PacketType {
    PACKET_TYPE_OK = 1,
    PACKET_TYPE_ERR = 2,
    PACKET_TYPE_EOF = 3,
    PACKET_TYPE_AUTHMOREDATA = 4,
    PACKET_TYPE_LOCALINFILE = 5,
    PACKET_TYPE_DATA = 6
};

struct Packet {
    uint32_t len{ 0 };
    uint8_t seq{ 0 };
    int8_t type{ 0 };
    std::string data;
};

namespace protocol {

const uint8_t VERSION = 10;

enum CapabilityFlags {
    CLIENT_LONG_PASSWORD = 0x00000001,
    CLIENT_FOUND_ROWS = 0x00000002,
    CLIENT_LONG_FLAG = 0x00000004,
    CLIENT_CONNECT_WITH_DB = 0x00000008,
    CLIENT_NO_SCHEMA = 0x00000010,
    CLIENT_COMPRESS = 0x00000020,
    CLIENT_ODBC = 0x00000040,
    CLIENT_LOCAL_FILES = 0x00000080,
    CLIENT_IGNORE_SPACE = 0x00000100,
    CLIENT_PROTOCOL_41 = 0x00000200,
    CLIENT_INTERACTIVE = 0x00000400,
    CLIENT_SSL = 0x00000800,
    CLIENT_IGNORE_SIGPIPE = 0x00001000,
    CLIENT_TRANSACTIONS = 0x00002000,
    CLIENT_RESERVED = 0x00004000,
    CLIENT_SECURE_CONNECTION = 0x00008000,
    CLIENT_MULTI_STATEMENTS = 0x00010000,
    CLIENT_MULTI_RESULTS = 0x00020000,
    CLIENT_PS_MULTI_RESULTS = 0x00040000,
    CLIENT_PLUGIN_AUTH = 0x00080000,
    CLIENT_CONNECT_ATTRS = 0x00100000,
    CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA = 0x00200000,
    CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS = 0x00400000,
    CLIENT_SESSION_TRACK = 0x00800000,
    CLIENT_DEPRECATE_EOF = 0x01000000,
    CLIENT_OPTIONAL_RESULTSET_METADATA = 0x02000000,
    CLIENT_QUERY_ATTRIBUTES = 0x08000000,
    MULTI_FACTOR_AUTHENTICATION = 0x10000000,
    CLIENT_SSL_VERIFY_SERVER_CERT = 0x40000000,
};

constexpr uint32_t CLIENT_CAPABILITIES =
    CLIENT_LONG_PASSWORD |
    CLIENT_LONG_FLAG |
    CLIENT_PROTOCOL_41 |
    CLIENT_TRANSACTIONS |
    CLIENT_SECURE_CONNECTION |
    CLIENT_MULTI_STATEMENTS |
    CLIENT_MULTI_RESULTS |
    CLIENT_PS_MULTI_RESULTS |
    CLIENT_PLUGIN_AUTH |
    CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA |
    CLIENT_CONNECT_ATTRS |
    CLIENT_SESSION_TRACK |
    CLIENT_DEPRECATE_EOF;

enum class Command {
    COM_SLEEP = 0x00,
    COM_QUIT = 0x01,
    COM_INIT_DB = 0x02,
    COM_QUERY = 0x03,
    COM_FIELD_LIST = 0x04,
    COM_CREATE_DB = 0x05,
    COM_DROP_DB = 0x06,
    COM_REFRESH = 0x07,
    COM_SHUTDOWN = 0x08,
    COM_STATISTICS = 0x09,
    COM_PROCESS_INFO = 0x0a,
    COM_CONNECT = 0x0b,
    COM_PROCESS_KILL = 0x0c,
    COM_DEBUG = 0x0d,
    COM_PING = 0x0e,
    COM_TIME = 0x0f,
    COM_DELAYED_INSERT = 0x10,
    COM_CHANGE_USER = 0x11,
    COM_RESET_CONNECTION = 0x12,
    COM_DAEMON = 0x13,
};

enum class StatusFlags {
    SERVER_STATUS_IN_TRANS = 0x0001, //a transaction is active
    SERVER_STATUS_AUTOCOMMIT = 0x0002, //auto-commit is enabled
    SERVER_MORE_RESULTS_EXISTS = 0x0008, //
    SERVER_STATUS_NO_GOOD_INDEX_USED = 0x0010, //
    SERVER_STATUS_NO_INDEX_USED = 0x0020, //
    SERVER_STATUS_CURSOR_EXISTS = 0x0040, //Used by Binary Protocol Resultset to signal that COM_STMT_FETCH must be used to fetch the row-data.
    SERVER_STATUS_LAST_ROW_SENT = 0x0080, //
    SERVER_STATUS_DB_DROPPED = 0x0100, //
    SERVER_STATUS_NO_BACKSLASH_ESCAPES = 0x0200, //
    SERVER_STATUS_METADATA_CHANGED = 0x0400, //
    SERVER_QUERY_WAS_SLOW = 0x0800, //
    SERVER_PS_OUT_PARAMS = 0x1000, //
    SERVER_STATUS_IN_TRANS_READONLY = 0x2000, //in a read-only transaction
    SERVER_SESSION_STATE_CHANGED = 0x4000, //connection state information has changed
};

enum class NullSafety {
    Nullable = 0,
    NonNull = 1
};

struct LengthEncodedInteger {
    uint64_t value;
    bool ParseFromString(const std::string& data, size_t& pos);
    bool SerializeToString(std::string* output);
};

struct NulTerminatedString {
    std::string& value;
    NulTerminatedString(std::string& value_ref);
    bool ParseFromString(const std::string& data, size_t& pos);
    bool SerializeToString(std::string* output);
};


struct LengthEncodedString {
    std::string& value;
    LengthEncodedString(std::string& value_ref);
    bool ParseFromString(const std::string& data, size_t& pos, NullSafety null_safety = NullSafety::NonNull);
    bool SerializeToString(std::string* output);
};

struct InitialHandshake_Packet {
    uint8_t protocol_version{ 0 };
    std::string server_version;
    uint32_t thread_id{ 0 };
    std::string auth_plugin_data;
    uint32_t capability_flags{ 0 };
    uint8_t character_set{ 0 };
    uint16_t status_flags{ 0 };
    std::string auth_plugin_name;

    bool ParseFromString(const std::string& data);
};

struct HandshakeResponse_Packet {
    uint32_t capability_flags{ 0 };
    uint32_t max_packet_size{ 0 };
    uint8_t character_set{ 0 };
    std::string username;
    std::string auth_response;
    std::string database;
    std::string auth_plugin_name;
    std::map<std::string, std::string> key_values;

    bool SerializeToString(std::string* output);
};

struct SSLRequest {
    uint32_t capability_flags{ 0 };
    uint32_t max_packet_size{ 0 };
    uint8_t character_set{ 0 };

    bool SerializeToString(std::string* output);
};

struct Err_Packet {
    int error_code{ 0 };
    char sql_state_marker{'#'};
    std::string sql_state;
    std::string error_message;

    bool ParseFromString(const std::string& data, uint32_t capability_flags);
};

struct OK_Packet {
    int affected_rows{ 0 };
    int last_insert_id{ 0 };
    int status_flags{ 0 };
    int warnings{ 0 };
    std::string info;
    std::string session_state_changes;

    bool ParseFromString(const std::string& data, uint32_t capability_flags);
};

struct EOF_Packet {
    int status_flags{ 0 };
    int warnings{ 0 };
    bool ParseFromString(const std::string& data, uint32_t capability_flags);
};
struct ColumnDefinition {
    std::string catalog;
    std::string schema;
    std::string table;
    std::string org_table;
    std::string name;
    std::string org_name;
    uint16_t character_set{ 0 };
    uint32_t column_length{ 0 };
    uint8_t type{ 0 };
    uint16_t flags{ 0 };
    uint8_t decimals{ 0 };
    std::string default_values;

    bool ParseFromString(const std::string& data, size_t& pos, Command cmd = Command::COM_QUERY);
};

struct AuthSwitchRequest {
    std::string auth_method_name;
    std::string auth_method_data;
    bool ParseFromString(const std::string& data);
};

struct AuthMoreData {
    std::string payload;
    bool ParseFromString(const std::string& data);
};

const char FAST_AUTH_SUCCESS = '\3';

const char PERFORM_FULL_AUTHENTICATION = '\4';
}
}