#include "redis_codec.h"
namespace redis {

bool RedisCodec::Decode(tinynet::net::SocketPtr& sock, RedisReply* reply) {
    if (sock->rbuf()->empty()) return false; //At least one character needed

    if (reply->type == REDIS_REPLY_ARRAY && reply->integer >= 0) {
        for (size_t i = (size_t)reply->integer; i < reply->element.size(); ++i) {
            if (!Decode(sock, &reply->element[i])) {
                return false;
            }
            ++reply->integer;
        }
        reply->integer = -1;
        return true;
    }
    reply->clear();
    char ch;
    size_t len, pos;
    tinynet::IBufferStream stream(sock->rbuf());
    ch = stream.read_byte();
    switch (ch) {
    case '+':
    case '-': {
        pos = stream.find_crlf();
        if (pos == tinynet::IOBuffer::npos)
            break;

        size_t len = pos - stream.tellg();
        reply->str.resize(len);
        stream.read(&reply->str[0], len);
        stream.seekg(pos + 2); //Remove trailing CRLF
        stream.cut();
        reply->type = ch == '+' ? REDIS_REPLY_STATUS : REDIS_REPLY_ERROR;
        return true;
    }
    case ':':
    case '$':
    case '*':
        pos = stream.find_crlf();
        if (pos == tinynet::IOBuffer::npos)
            break;

        int64_t value;
        len = pos - stream.tellg();
        if (!protocol::ReadInteger(stream, len, &value)) {
            sock->SetError(tinynet::ERROR_REDIS_READINGREPLY);
            return false;
        }
        pos += 2; //Skip CRLF bytes
        stream.seekg(pos);
        len = value;
        if (ch == ':') {
            reply->type = REDIS_REPLY_INTEGER;
            reply->integer = value;
            stream.cut();
            return true;
        } else if (ch == '$') {
            if (value < 0) {
                reply->type = REDIS_REPLY_NIL;
                stream.cut();
                return true;
            }
            len = stream.rdbuf()->size() - pos;
            size_t n = (int64_t)value;
            if (len < n + 2) {
                return false;
            }
            reply->type = REDIS_REPLY_STRING;
            reply->str.resize(n);
            stream.read(&reply->str[0], n);
            stream.seekg(pos + n + 2); //Skip CRLF bytes
            stream.cut();
            return true;
        } else {
            reply->type = REDIS_REPLY_ARRAY;
            stream.cut();
            if (value <= 0) {
                reply->integer = -1;
                return true;
            }
            reply->element.resize(value);
            reply->integer = 0;
            for (size_t i = 0; i < reply->element.size(); ++i) {
                if (!Decode(sock, &reply->element[i])) {
                    return false;
                }
                ++reply->integer;
            }
            reply->integer = -1;
            return true;
        }
    default:
        break;
    }
    return false;
}

RedisReply* RedisCodec::Read(tinynet::net::SocketPtr& sock) {
    return Decode(sock, &reply_) ? &reply_ : nullptr;
}


void RedisCodec::Write(tinynet::net::SocketPtr& sock, const tinynet::string_view& cmd) {
    protocol::WriteCommand(sock->wbuf(), cmd.data(), cmd.size());
    sock->Flush();
}

void RedisCodec::Write(tinynet::net::SocketPtr& sock, const std::vector<tinynet::string_view>& iovs) {
    protocol::WriteCommand(sock->wbuf(), iovs);
    sock->Flush();
}
}
