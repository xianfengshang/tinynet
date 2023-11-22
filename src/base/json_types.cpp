#include "json_types.h"
namespace tinynet {
namespace json {
Allocator g_allocator;

std::string tojson(Value& value, bool pretty) {
    rapidjson::MemoryBuffer s;
    if (pretty) {
        rapidjson::PrettyWriter<rapidjson::MemoryBuffer> writer(s);
        value.Accept(writer);
    } else {
        rapidjson::Writer<rapidjson::MemoryBuffer> writer(s);
        value.Accept(writer);
    }
    return std::string(s.GetBuffer(), s.GetSize());
}

}
}