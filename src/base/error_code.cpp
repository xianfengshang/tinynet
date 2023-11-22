// Auto generate, don not modify!
#include "error_code.h"
#include "google/protobuf/descriptor.h"
using namespace tinynet;
const char* tinynet_strerror(int err) {
    thread_local static char buffer[1024] = { 0 };
    auto desc = ErrorCode_descriptor();
    auto value = desc->FindValueByNumber(err);
    if (!value) {
        return "Unknown error";
    }
    google::protobuf::SourceLocation source;
    if (desc->GetSourceLocation(&source) && source.trailing_comments.size() > 0) {
        snprintf(buffer, sizeof(buffer),source.trailing_comments.c_str());
    } else {
        snprintf(buffer, sizeof(buffer), value->name().c_str());
    }
    return buffer;
}
