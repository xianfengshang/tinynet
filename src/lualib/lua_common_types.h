#pragma once
#include <string>
namespace tinynet {
namespace lua {

struct ClientOptions {
    std::string url;
    int keepalive_timeout{ 0 };
    int ping_interval{ 0 };
};
struct ServerOptions {
    std::string listen_url;
    std::string listen_ip;
    int listen_port{ 0 };
    int keepalive_timeout{ 0 };
    bool reuseport{ false };
    bool ipv6only{ false };
};

struct JsonCodecOptions {
    int encode_max_depth{ 0 };
    int decode_max_depth{ 0 };
    bool order_map_entries{ false };
    bool empty_table_as_array{ false };
    bool pretty_format{ false };
    bool encode_as_bytes{ false };
};
struct ProtobufCodecOptions {
    bool encode_as_bytes{ false };
    bool bytes_as_string{ false };
};
struct MemoryStats {
    size_t allocated{ 0 };
    size_t active{ 0 };
    size_t resident{ 0 };
    size_t mapped{ 0 };
    size_t retained{ 0 };
};
}
}