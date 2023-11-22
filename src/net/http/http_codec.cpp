// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "http_codec.h"
#include "http_parser.h"
#include "logging/logging.h"
#include "util/string_utils.h"
#include "base/io_buffer.h"
#include "base/error_code.h"

namespace tinynet {
namespace http {

static struct http_parser_settings settings = {
    HttpCodec::s_on_message_begin,
    HttpCodec::s_on_url,
    0,
    HttpCodec::s_on_header_field,
    HttpCodec::s_on_header_value,
    HttpCodec::s_on_headers_complete,
    HttpCodec::s_on_body,
    HttpCodec::s_on_message_complete,
    0,
    0
};

HttpCodec::HttpCodec():
    header_status_(NOTHING),
    unzip_(false),
    zfail_(false),
    zinit_(false) {
    http_parser_init(&parser_, HTTP_BOTH);
    parser_.data = this;
}

bool HttpCodec::Read(net::SocketPtr& sock, std::vector<std::unique_ptr<HttpMessage>> *outputs) {
    size_t parsed  = http_parser_execute(&parser_, &settings, sock->rbuf()->begin(), sock->rbuf()->size());
    if (parser_.http_errno) {
        sock->SetError(ERROR_HTTP_REQUEST);
        return false;
    }
    sock->rbuf()->consume(parsed);
    if (outputs) {
        outputs->swap(messages_);
        return outputs->size() > 0;
    }
    return false;
}

void HttpCodec::Write(net::SocketPtr& sock, const HttpMessage *msg) {
    if (!msg) {
        return;
    }
    std::string buffer;
    if (msg->type == http::HTTP_REQUEST) {
        StringUtils::Format(buffer, "%s %s%s HTTP/1.1", msg->method.c_str(), msg->path.c_str(), msg->query.c_str());
    } else {
        StringUtils::Format(buffer, "HTTP/1.1 %d ", msg->statusCode);

        if (msg->statusMessage.empty()) {
            buffer.append(http_status_str((http_status)msg->statusCode));
        } else {
            buffer.append(msg->statusMessage);
        }
    }
    buffer.append("\r\n");

    if (msg->headers.find("Content-Length") == msg->headers.end()) {
        StringUtils::Format(buffer, "Content-Length: %zd\r\n", msg->body.size());
    }
    if (msg->headers.find("Connection") == msg->headers.end()) {
        buffer.append("Connection: Keep-Alive\r\n");
    }


    for (auto &header : msg->headers) {
        buffer.append(header.first);
        buffer.append(": ");
        buffer.append(header.second);
        buffer.append("\r\n");
    }
    buffer.append("\r\n");
    buffer.append(msg->body);

    sock->Write(buffer.data(), buffer.size());
}

void HttpCodec::Write(net::SocketPtr& sock, const ZeroCopyHttpMessage *msg) {
    if (!msg) {
        return;
    }
    std::string buffer;
    if (msg->type == http::HTTP_REQUEST) {
        StringUtils::Format(buffer, "%.*s %.*s%.*s HTTP/1.1", (int)msg->method.size(), msg->method.data(),
                            (int)msg->path.size(), msg->path.data(), (int)msg->query.size(), msg->query.data());
    } else {
        StringUtils::Format(buffer, "HTTP/1.1 %d ", msg->statusCode);

        if (msg->statusMessage.empty()) {
            buffer.append(http_status_str((http_status)msg->statusCode));
        } else {
            buffer.append(msg->statusMessage);
        }
    }
    buffer.append("\r\n");

    if (msg->headers.find("Content-Length") == msg->headers.end()) {
        StringUtils::Format(buffer, "Content-Length: %zd\r\n", msg->body.size());
    }
    if (msg->headers.find("Connection") == msg->headers.end()) {
        buffer.append("Connection: Close\r\n");
    }


    for (auto &header : msg->headers) {
        buffer.append(header.first.data(), header.first.size());
        buffer.append(": ");
        buffer.append(header.second.data(), header.second.size());
        buffer.append("\r\n");
    }
    buffer.append("\r\n");
    buffer.append(msg->body.data(), msg->body.size());

    sock->Write(buffer.data(), buffer.size());
}

int HttpCodec::on_message_begin() {
    message_.reset(new (std::nothrow) HttpMessage());
    return 0;
}

int HttpCodec::on_url(const char *at, size_t length) {
    if (message_) {
        message_->url.append(at, length);
    }
    return 0;
}

/*
* @see https://github.com/simsong/tcpflow/blob/master/src/scan_http.cpp
*/
int HttpCodec::on_header_field(const char *at, size_t length) {
    std::string field(at, length);
    std::transform(field.begin(), field.end(), field.begin(), ::tolower);

    switch (header_status_) {
    case NOTHING:
        // Allocate new buffer and copy callback data into it
        header_field_ = field;
        break;
    case VALUE:
        // New header started.
        // Copy current name,value buffers to headers
        // list and allocate new buffer for new name
        if (message_) {
            message_->headers[header_field_] = header_value_;
        }
        header_field_ = field;
        break;
    case FIELD:
        // Previous name continues. Reallocate name
        // buffer and append callback data to it
        header_field_.append(field);
        break;
    }
    header_status_ = FIELD;
    return 0;
}

int HttpCodec::on_header_value(const char *at, size_t length) {
    const std::string value(at, length);
    switch (header_status_) {
    case FIELD:
        //Value for current header started. Allocate
        //new buffer and copy callback data to it
        header_value_ = value;
        break;
    case VALUE:
        //Value continues. Reallocate value buffer
        //and append callback data to it
        header_value_.append(value);
        break;
    case NOTHING:
        // this shouldn't happen
        log_error("Internal error in http-parser");
        break;
    }
    header_status_ = VALUE;

    return 0;
}
int HttpCodec::on_headers_complete() {
    if (!message_) return 0;
    /* Add the most recently read header to the map, if any */
    if (header_status_ == VALUE) {
        message_->headers[header_field_] = header_value_;
        header_field_ = "";
    }
    std::string content_encoding(message_->headers["content-encoding"]);

    if ((content_encoding == "gzip" || content_encoding == "deflate")) {
        unzip_ = true;
    }
    message_->method = http_method_str((http_method)parser_.method);

    //parse url
    struct http_parser_url parser_url;
    http_parser_url_init(&parser_url);
    if (http_parser_parse_url(&message_->url[0], message_->url.length(), 0, &parser_url) == 0) {
        if (parser_url.field_set & (1 << UF_PATH)) {
            message_->path.assign(&message_->url[parser_url.field_data[UF_PATH].off], parser_url.field_data[UF_PATH].len);
        }
        if (parser_url.field_set & (1 << UF_QUERY)) {
            message_->query.assign(&message_->url[parser_url.field_data[UF_QUERY].off], parser_url.field_data[UF_QUERY].len);
        }
    }
    message_->type = parser_.type;
    return 0;
}

int HttpCodec::on_body(const char *at, size_t length) {
    if (length == 0) {
        return 0;
    }
    if (unzip_ == false) {
        if (message_) {
            message_->body.append(at, length);
        }
        return 0;
    }
    if (zfail_) {
        return 0;
    }
    char decompressed[10 * 1024];           // where decompressed data goes
    if (!zinit_) {
        memset(&zs_, 0, sizeof(zs_));
        zs_.next_in = (Bytef*)at;
        zs_.avail_in = (uInt)length;
        zs_.next_out = (Bytef*)decompressed;
        zs_.avail_out = sizeof(decompressed);

        int rv = inflateInit2(&zs_, 32 + MAX_WBITS);      /* 32 auto-detects gzip or deflate */
        if (rv != Z_OK) {
            /* fail! */
            log_error("decompression failed at stream initialization; rv=%d bad Content-Encoding?", rv);
            zfail_ = true;
            return 0;
        }
        zinit_ = true;                   // successfully initted
    } else {
        zs_.next_in = (Bytef*)at;
        zs_.avail_in = (uInt)length;
        zs_.next_out = (Bytef*)decompressed;
        zs_.avail_out = sizeof(decompressed);
    }

    /* iteratively decompress, writing each time */
    while (zs_.avail_in > 0) {
        /* decompress as much as possible */
        int rv = inflate(&zs_, Z_SYNC_FLUSH);

        if (rv == Z_STREAM_END) {
            /* are we done with the stream? */
            if (zs_.avail_in > 0) {
                /* ...no. */
                log_error("decompression completed, but with trailing garbage");
                return 0;
            }
        } else if (rv != Z_OK) {
            /* some other error */
            log_error("decompression failed (corrupted stream?)");
            zfail_ = true;               // ignore the rest of this stream
            return 0;
        }

        /* successful decompression, at least partly */
        /* write the result */
        int bytes_decompressed = sizeof(decompressed) - zs_.avail_out;
        if (message_) {
            message_->body.append(decompressed, bytes_decompressed);
        }

        /* reset the buffer for the next iteration */
        zs_.next_out = (Bytef*)decompressed;
        zs_.avail_out = sizeof(decompressed);
    }
    return 0;
}

int HttpCodec::on_message_complete() {
    messages_.emplace_back(std::move(message_));
    if (header_field_.size() > 0) {
        std::string empty;
        header_field_.swap(empty);
    }
    if (header_value_.size() > 0) {
        std::string empty;
        header_value_.swap(empty);
    }
    header_status_ = NOTHING;
    unzip_ = false;
    if (zinit_) {
        inflateEnd(&zs_);
        zinit_ = false;
    }
    zfail_ = false;
    return 0;
}

}
}
