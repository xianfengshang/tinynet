// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "mysql_protocol.h"
#include "base/coding.h"
#include <algorithm>

namespace mysql {
namespace protocol {

bool LengthEncodedInteger::ParseFromString(const std::string& data, size_t& pos) {
    uint8_t first = static_cast<uint8_t>(data[pos]);
    if (first < 0xfb) {
        value = first;
        pos += 1;
        return true;
    }
    if (first == 0xfc) {
        value = tinynet::DecodeFixed16(&data[pos + 1]);
        pos += 3;
        return true;
    }
    if (first == 0xfd) {
        value = tinynet::DecodeFixed24(&data[pos + 1]);
        pos += 4;
        return true;
    }
    if (first == 0xfe) {
        value = tinynet::DecodeFixed64(&data[pos + 1]);
        pos += 9;
        return true;
    }
    value = 0;
    return false;
}

bool LengthEncodedInteger::SerializeToString(std::string* output) {
    if (value < 251) {
        output->append(1, static_cast<char>(value));
        return true;
    }
    char buf[sizeof(int64_t)];
    if (value >= 251 && value < 65536) {
        output->append(1, (char)0xfc);
        tinynet::EncodeFixed16(buf, static_cast<uint16_t>(value));
        output->append(buf, 2);
        return true;
    }
    if (value >= 65536 && value < 16777216) {
        output->append(1, (char)0xfd);
        tinynet::EncodeFixed24(buf, static_cast<uint32_t>(value));
        output->append(buf, 3);
        return true;
    }
    output->append(1, (char)0xfe);
    tinynet::EncodeFixed64(buf, value);
    output->append(buf, 8);
    return true;
}

NulTerminatedString::NulTerminatedString(std::string& value_ref) :
    value(value_ref) {
}

bool NulTerminatedString::ParseFromString(const std::string& data, size_t& pos) {
    size_t null_pos = data.find_first_of('\0', pos);
    if (null_pos == std::string::npos) {
        return false;
    }
    if (pos == null_pos) {
        pos = pos + 1;
        return true;
    }
    value = data.substr(pos, null_pos - pos);
    pos += value.length() + 1;
    return true;
}

bool NulTerminatedString::SerializeToString(std::string* output) {
    if (!value.empty()) {
        output->append(value);
    }
    output->append(1, '\0');
    return true;
}

LengthEncodedString::LengthEncodedString(std::string& value_ref) :
    value(value_ref) {
}

bool LengthEncodedString::ParseFromString(const std::string& data, size_t& pos, NullSafety null_safety /* = NullSafety::NonNull */) {
    LengthEncodedInteger length;
    if (!length.ParseFromString(data, pos)) {
        if (null_safety == NullSafety::Nullable && static_cast<uint8_t>(data[pos]) == 0xfb ) { // In result set row, NULL is sent as 0xfb
            pos += sizeof(uint8_t);
            return true;
        }
        return false;
    }
    value = data.substr(pos, (size_t)length.value);
    pos += value.length();
    return true;
}

bool LengthEncodedString::SerializeToString(std::string* output) {
    LengthEncodedInteger length;
    length.value = value.length();
    length.SerializeToString(output);
    output->append(value);
    return true;
}

bool Err_Packet::ParseFromString(const std::string& data, uint32_t capability_flags) {
    size_t pos = 1;
    error_code = tinynet::DecodeFixed16(&data[pos]);
    pos += sizeof(uint16_t);
    if (capability_flags & protocol::CLIENT_PROTOCOL_41) {
        sql_state_marker = data[pos];
        pos += 1;
        sql_state = data.substr(pos, 5);
        pos += 5;
    }
    error_message = data.substr(pos);
    return true;
}

bool OK_Packet::ParseFromString(const std::string& data, uint32_t capability_flags) {
    if (data.length() < 7) {
        return false;
    }
    size_t pos = 1;
    LengthEncodedInteger value;
    if (!value.ParseFromString(data, pos)) {
        return false;
    }
    affected_rows = static_cast<decltype(affected_rows)> (value.value);
    if (!value.ParseFromString(data, pos)) {
        return false;
    }
    last_insert_id = static_cast<decltype(last_insert_id)> (value.value);
    if (capability_flags & CLIENT_PROTOCOL_41) {
        status_flags = tinynet::DecodeFixed16(&data[pos]);
        pos += sizeof(uint16_t);
        warnings = tinynet::DecodeFixed16(&data[pos]);
        pos += sizeof(uint16_t);
    } else if (capability_flags & CLIENT_TRANSACTIONS) {
        status_flags = tinynet::DecodeFixed16(&data[pos]);
        pos += sizeof(uint16_t);
    }
    if (pos >= data.size()) {
        return true;
    }
    if (capability_flags & CLIENT_SESSION_TRACK) {
        LengthEncodedString info_ref(info);
        if (!info_ref.ParseFromString(data, pos)) {
            return false;
        }
        if (status_flags & (uint32_t)StatusFlags::SERVER_SESSION_STATE_CHANGED) {
            LengthEncodedString session_state_changes_ref(session_state_changes);
            if (!session_state_changes_ref.ParseFromString(data, pos)) {
                return false;
            }
        }
    } else {
        info = data.substr(pos);
    }
    return true;
}

bool EOF_Packet::ParseFromString(const std::string& data, uint32_t capability_flags) {
    if (data.length() >  5) {
        return false;
    }
    size_t pos = 1;
    if (capability_flags & CLIENT_PROTOCOL_41) {
        warnings = tinynet::DecodeFixed16(&data[pos]);
        pos += sizeof(uint16_t);
        status_flags = tinynet::DecodeFixed16(&data[pos]);
        pos += sizeof(uint16_t);
    }
    return true;
}

bool InitialHandshake_Packet::ParseFromString(const std::string& data) {
    size_t pos = 0;
    protocol_version = static_cast<uint8_t>(data[pos]);
    pos += sizeof(uint8_t);
    NulTerminatedString server_version_ref(server_version);
    server_version_ref.ParseFromString(data, pos);
    thread_id = tinynet::DecodeFixed32(&data[pos]);
    pos += sizeof(uint32_t);
    auth_plugin_data = data.substr(pos, 8);
    pos += 8;
    pos += sizeof(uint8_t); //skip filter
    uint16_t capability_low = tinynet::DecodeFixed16(&data[pos]);
    pos += sizeof(uint16_t);
    character_set = static_cast<uint8_t>(data[pos]);
    pos += sizeof(uint8_t);
    status_flags = tinynet::DecodeFixed16(&data[pos]);
    pos += sizeof(uint16_t);
    uint16_t capability_high = tinynet::DecodeFixed16(&data[pos]);
    pos += sizeof(uint16_t);
    capability_flags = (capability_high << 16) | capability_low;
    uint8_t auth_plugin_data_len = static_cast<uint8_t>(data[pos]);
    pos += sizeof(uint8_t);
    //reserved
    pos += 10;
    if (capability_flags & protocol::CLIENT_SECURE_CONNECTION) {
        uint8_t	auth_plugin_data_part_2_len = 13;
        if (auth_plugin_data_len > 8) {
            auth_plugin_data_part_2_len = (std::max)(auth_plugin_data_part_2_len, (uint8_t)(auth_plugin_data_len - 8));
        }
        auth_plugin_data.append(data.substr(pos, auth_plugin_data_part_2_len - 1));
        pos += auth_plugin_data_part_2_len;
    }
    if (capability_flags & protocol::CLIENT_PLUGIN_AUTH) {
        NulTerminatedString auth_plugin_name_ref(auth_plugin_name);
        if (!auth_plugin_name_ref.ParseFromString(data, pos)) {
            return false;
        }
    }
    return true;
}

bool HandshakeResponse_Packet::SerializeToString(std::string* output) {
    char buf[sizeof(uint32_t)];
    if (!database.empty())
        capability_flags |= protocol::CLIENT_CONNECT_WITH_DB;
    else
        capability_flags &= ~protocol::CLIENT_CONNECT_WITH_DB;
    if (!auth_plugin_name.empty())
        capability_flags |= protocol::CLIENT_PLUGIN_AUTH;
    else
        capability_flags &= ~protocol::CLIENT_PLUGIN_AUTH;
    if (!key_values.empty())
        capability_flags |= protocol::CLIENT_CONNECT_ATTRS;
    else
        capability_flags &= ~protocol::CLIENT_CONNECT_ATTRS;

    tinynet::EncodeFixed32(buf, capability_flags);
    output->append(buf, sizeof(buf));
    tinynet::EncodeFixed32(buf, max_packet_size);
    output->append(buf, sizeof(buf));
    output->append(1, character_set);
    output->append(23, '\0');
    NulTerminatedString username_ref(username);
    username_ref.SerializeToString(output);

    if (capability_flags & protocol::CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA) {
        LengthEncodedString auth_response_ref(auth_response);
        auth_response_ref.SerializeToString(output);
    } else if (capability_flags & protocol::CLIENT_SECURE_CONNECTION) {
        output->append(1, (char)auth_response.length());
        output->append(auth_response);
    } else {
        if (auth_response.length() == 1 && (auth_response[0] == 1 || auth_response[0] == 2)) {
            output->append(1, auth_response[0]);
        } else {
            NulTerminatedString auth_response_ref(auth_response);
            auth_response_ref.SerializeToString(output);
        }
    }

    if (capability_flags & protocol::CLIENT_CONNECT_WITH_DB) {
        NulTerminatedString database_ref(database);
        database_ref.SerializeToString(output);
    }
    if (capability_flags & protocol::CLIENT_PLUGIN_AUTH) {
        NulTerminatedString auth_plugin_name_ref(auth_plugin_name);
        auth_plugin_name_ref.SerializeToString(output);
    }
    if (capability_flags & protocol::CLIENT_CONNECT_ATTRS) {
        LengthEncodedInteger length;
        length.value = key_values.size();
        length.SerializeToString(output);
        for (auto& kv_pair : key_values) {
            LengthEncodedString key(const_cast<std::string&>(kv_pair.first));
            LengthEncodedString value(const_cast<std::string&>(kv_pair.second));
            key.SerializeToString(output);
            value.SerializeToString(output);
        }
    }
    return true;
}

bool SSLRequest::SerializeToString(std::string* output) {
    char buf[sizeof(uint32_t)];
    tinynet::EncodeFixed32(buf, capability_flags);
    output->append(buf, sizeof(buf));
    tinynet::EncodeFixed32(buf, max_packet_size);
    output->append(buf, sizeof(buf));
    output->append(1, character_set);
    output->append(23, '\0');
    return true;
}

bool ColumnDefinition::ParseFromString(const std::string& data, size_t& pos, Command cmd) {
    LengthEncodedString catalog_ref(catalog);
    if (!catalog_ref.ParseFromString(data, pos)) {
        return false;
    }
    LengthEncodedString schema_ref(schema);
    if (!schema_ref.ParseFromString(data, pos)) {
        return false;
    }
    LengthEncodedString table_ref(table);
    if (!table_ref.ParseFromString(data, pos)) {
        return false;
    }
    LengthEncodedString org_table_ref(org_table);
    if (!org_table_ref.ParseFromString(data, pos)) {
        return false;
    }
    LengthEncodedString name_ref(name);
    if (!name_ref.ParseFromString(data, pos)) {
        return false;
    }
    LengthEncodedString org_name_ref(org_name);
    if (!org_name_ref.ParseFromString(data, pos)) {
        return false;
    }
    pos += 1; //ignore filter
    character_set = tinynet::DecodeFixed16(&data[pos]);
    pos += sizeof(uint16_t);
    column_length = tinynet::DecodeFixed32(&data[pos]);
    pos += sizeof(uint32_t);
    type = static_cast<decltype(type)>(data[pos]);
    pos += sizeof(uint8_t);
    flags = tinynet::DecodeFixed16(&data[pos]);
    pos += sizeof(uint16_t);
    decimals = static_cast<decltype(decimals)>(data[pos]);
    pos += sizeof(uint8_t);
    if (cmd == Command::COM_FIELD_LIST) {
        LengthEncodedString default_values_ref(default_values);
        if (!default_values_ref.ParseFromString(data, pos)) {
            return false;
        }
    }
    return true;
}

bool AuthSwitchRequest::ParseFromString(const std::string& data) {
    if (data.length() == 0 || (uint8_t)data[0] != 0xfe) {
        return false;
    }
    size_t pos = 0;
    //skip status
    pos += 1;
    if (pos >= data.length()) {
        auth_method_name = "mysql_old_password";
        return true;
    }
    NulTerminatedString auth_method_name_ref(auth_method_name);
    if (!auth_method_name_ref.ParseFromString(data, pos)) {
        return false;
    }
    auth_method_data = data.substr(pos);
    return true;
}

bool AuthMoreData::ParseFromString(const std::string& data) {
    size_t pos = 0;
    //skip status
    pos += 1;
    payload = data.substr(pos);
    return true;
}

}
}
