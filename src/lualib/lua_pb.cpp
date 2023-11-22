// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_pb.h"
#include "rpc/protobuf_manager.h"
#include "lua_helper.h"
#include "util/string_utils.h"
#include "lua_compat.h"
#include "lua_script.h"
#include "lua_bytes.h"
#include "lua_common_types.h"
#include "lua_types.h"
#include "lua_proto_types.h"

#define PROTOBUF_META_TABLE "protobuf_meta_table"
using namespace tinynet;
#define LUA_TNIL_MASK 0x00000001
#define LUA_TBOOLEAN_MASK 0x00000002
#define LUA_TLIGHTUSERDATA_MASK 0x00000004
#define LUA_TNUMBER_MASK 0x00000008
#define LUA_TSTRING_MASK 0x00000010
#define LUA_TTABLE_MASK  0x00000020
#define LUA_TFUNCTION_MASK 0x00000040
#define LUA_TUSERDATA_MASK 0x00000080
#define LUA_TTHREAD_MASK 0x00000100


static tinynet::rpc::ProtobufManager* luaL_checkpb(lua_State *L, int idx) {
    return (tinynet::rpc::ProtobufManager*)luaL_checkudata(L, idx, PROTOBUF_META_TABLE);
}

//A codec witch encodes lua table to a google protobuf binary data stream and
//decodes a google protobuf binary data stream to lua table
class LuaProtobufCodec {
  public:
    LuaProtobufCodec(lua_State* state, tinynet::rpc::ProtobufManager* protobuf, const tinynet::lua::ProtobufCodecOptions& opts);
    ~LuaProtobufCodec();
  public:
    //Get codec error message
    const std::string& get_errors() const {
        return errors_;
    }
    //Check if some errors occurred
    bool HasError() const {
        return error_num_ > 0;
    }
    //Encodes lua table to a google protobuf binary data stream.
    //Return 0 means encoded successfully and non zero means some error occurred.
    //Detail errors can be retrieved by calling get_errors
    int Encode(google::protobuf::Message* message);

    //Decodes a google protobuf binary data stream to lua table.
    //return 0 means encoded successfully and non zero means some error occurred.
    //Detail errors can be retrieved by calling get_errors
    int Decode(google::protobuf::Message* message);
  private:
    void AddError(const std::string& err);
    void ClearError();
  private:
    static void encode_message(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                               google::protobuf::Message* message);

    static void encode_field(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                             google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field);

    template<google::protobuf::FieldDescriptor::Type T>
    static void encode_field_value(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                                   google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field);

    static void encode_repeated(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                                google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field);

    static void encode_repeated_field(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                                      google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field);

    template<google::protobuf::FieldDescriptor::Type T>
    static void encode_repeated_value(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                                      google::protobuf::Message *message, const google::protobuf::FieldDescriptor *field);

    typedef void (*value_encoder)(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                                  google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field);


  private:
    static const google::protobuf::Message* ReflectionGetMessage(const google::protobuf::Reflection* reflection,
            const google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field);

    static void decode_message(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                               const google::protobuf::Message* message);

    static void decode_field(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                             const google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field);

    template<google::protobuf::FieldDescriptor::Type T>
    static void decode_field_value(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                                   const google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field, int index);

    static void decode_repeated(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                                const google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field);

    template<google::protobuf::FieldDescriptor::Type T>
    static void decode_repeated_value(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                                      const google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field, int index);

    typedef void (*value_decoder)(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,
                                  const google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field, int index);

  public:
    struct LuaProtobufMappingItem {
        int type; //Expected type
        int mask; //Allowed type masks.eg, Number 123 and string  "123"  are valid lua types to convert to google::protobuf::Descriptor::TYPE_INT32
        value_encoder field_value_encoder;
        value_encoder repeated_value_encoder;
        value_decoder field_value_decoder;
        value_decoder repeated_value_decoder;
    };

    static LuaProtobufMappingItem protobuf_lua_mapping[google::protobuf::FieldDescriptor::MAX_TYPE + 1];
  public:
    tinynet::rpc::ProtobufManager* get_protobuf() {
        return protobuf_;
    }
    const tinynet::lua::ProtobufCodecOptions& get_opts() {
        return opts_;
    }
  public:
    lua_State* L;
  private:
    tinynet::rpc::ProtobufManager* protobuf_;
    std::string errors_;
    int error_num_;
    tinynet::lua::ProtobufCodecOptions opts_;
};
#define DEFINE_ENCODER(NAME,TYPE)\
	template<>\
	void LuaProtobufCodec::encode_##NAME##_value<google::protobuf::FieldDescriptor::TYPE>(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,\
														 google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field)

#define ENCODER_FUNC(NAME,TYPE)\
	LuaProtobufCodec::encode_##NAME##_value<google::protobuf::FieldDescriptor::TYPE>

#define DEFINE_DECODER(NAME,TYPE)\
	template<>\
	void LuaProtobufCodec::decode_##NAME##_value<google::protobuf::FieldDescriptor::TYPE>(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection,\
														 const google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field, int index)

#define DECODER_FUNC(NAME,TYPE)\
	LuaProtobufCodec::decode_##NAME##_value<google::protobuf::FieldDescriptor::TYPE>


LuaProtobufCodec::LuaProtobufCodec(lua_State* state, tinynet::rpc::ProtobufManager* protobuf, const tinynet::lua::ProtobufCodecOptions& opts) :
    L(state),
    protobuf_(protobuf),
    error_num_(0),
    opts_(opts) {
}

LuaProtobufCodec::~LuaProtobufCodec() = default;

void LuaProtobufCodec::AddError(const std::string& err) {
    errors_.append(err);
    errors_.append(1, '\n');
    error_num_++;
}

void LuaProtobufCodec::ClearError() {
    if (errors_.length() > 0) {
        errors_.clear();
        errors_.shrink_to_fit();
    }
    error_num_ = 0;
}

int LuaProtobufCodec::Encode(google::protobuf::Message* message) {
    ClearError();
    encode_message(this, message->GetReflection(), message);
    return error_num_;
}

int LuaProtobufCodec::Decode(google::protobuf::Message* message) {
    ClearError();
    decode_message(this, message->GetReflection(), message);
    return error_num_;
}

void LuaProtobufCodec::encode_message(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection, google::protobuf::Message* message) {
    const google::protobuf::Descriptor *descriptor = message->GetDescriptor();
    int field_count = descriptor->field_count();
    for (int i = 0; i < field_count; ++i) {
        const google::protobuf::FieldDescriptor *field = descriptor->field(i);
        lua_getfield(codec->L, -1, field->name().c_str());
        encode_field(codec, reflection, message, field);
        lua_pop(codec->L, 1);
    }
    //extensions
    int range_count = descriptor->extension_range_count();
    for (int i = 0; i < range_count; ++i) {
        const google::protobuf::Descriptor::ExtensionRange* range = descriptor->extension_range(i);
        for (int n = range->start; n < range->end; ++n) {
            const google::protobuf::FieldDescriptor *field = reflection->FindKnownExtensionByNumber(n);
            if (field) {
                lua_getfield(codec->L, -1, field->name().c_str());
                encode_field(codec, reflection, message, field);
                lua_pop(codec->L, 1);
            }
        }
    }
}

void LuaProtobufCodec::encode_field(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection, google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field) {
    int ltype = lua_type(codec->L, -1);
    if (field->is_repeated()) {
        if (ltype != LUA_TTABLE) {
            if (ltype != LUA_TNIL) {
                std::string err;
                StringUtils::Format(err, "%s.%s table expected, but got %s", message->GetTypeName().c_str(), field->name().c_str(), lua_typename(codec->L, ltype));
                codec->AddError(err);
            }
            return;
        }
        encode_repeated(codec, reflection, message, field);
        return;
    }
    google::protobuf::FieldDescriptor::Type type = field->type();

    LuaProtobufMappingItem& item = codec->protobuf_lua_mapping[type];
    if ((item.mask & (1 << ltype)) == 0) {
        if (field->is_required()) {
            std::string err;
            StringUtils::Format(err, "%s.%s %s expected, but got %s", message->GetTypeName().c_str(), field->name().c_str(),
                                lua_typename(codec->L, item.type), lua_typename(codec->L, ltype));
            codec->AddError(err);
        }
        return;
    }
    item.field_value_encoder(codec, reflection, message, field);
}

void LuaProtobufCodec::encode_repeated(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection, google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field) {
#if (LUA_VERSION_NUM >= 503)
    size_t size = lua_rawlen(codec->L, -1);
#else
    size_t size = lua_objlen(codec->L, -1);
#endif
    for (size_t i = 0; i < size; ++i) {
        lua_pushnumber(codec->L, static_cast<lua_Number>(i + 1));
        lua_rawget(codec->L, -2);
        encode_repeated_field(codec, reflection, message, field);
        lua_pop(codec->L, 1);
    }
}

void LuaProtobufCodec::encode_repeated_field(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection, google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field) {
    int ltype = lua_type(codec->L, -1);

    google::protobuf::FieldDescriptor::Type type = field->type();

    LuaProtobufMappingItem& item = codec->protobuf_lua_mapping[type];
    if ((item.mask & (1 << ltype)) == 0) {
        std::string err;
        StringUtils::Format(err, "%s.%s %s expected, but got %s", message->GetTypeName().c_str(), field->name().c_str(),
                            lua_typename(codec->L, item.type), lua_typename(codec->L, ltype));
        codec->AddError(err);
        return;
    }
    item.repeated_value_encoder(codec, reflection, message, field);
}

const google::protobuf::Message* LuaProtobufCodec::ReflectionGetMessage(const google::protobuf::Reflection* reflection, const google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field) {
#pragma push_macro("GetMessage")
#undef GetMessage
    return &(reflection->GetMessage(*message, field));
#pragma pop_macro("GetMessage")
}

void LuaProtobufCodec::decode_message(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection, const google::protobuf::Message* message) {
    const google::protobuf::Descriptor *descriptor = message->GetDescriptor();
    lua_newtable(codec->L);
    int field_count = descriptor->field_count();
    for (int i = 0; i < field_count; ++i) {
        const google::protobuf::FieldDescriptor *field = descriptor->field(i);
        decode_field(codec, reflection, message, field);
    }
    //extensions
    int range_count = descriptor->extension_range_count();
    for (int i = 0; i < range_count; ++i) {
        const google::protobuf::Descriptor::ExtensionRange* range = descriptor->extension_range(i);
        for (int n = range->start; n < range->end; ++n) {
            const google::protobuf::FieldDescriptor *field = reflection->FindKnownExtensionByNumber(n);
            if (field) {
                decode_field(codec, reflection, message, field);
            }
        }
    }
}

void LuaProtobufCodec::decode_field(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection, const google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field) {
    if (field->is_optional() && (!reflection->HasField(*message, field)) && (!field->has_default_value())) {
        return;
    }
    if (field->is_repeated()) {
        decode_repeated(codec,reflection,  message, field);
        return;
    }

    google::protobuf::FieldDescriptor::Type type = field->type();
    LuaProtobufMappingItem& item = codec->protobuf_lua_mapping[type];
    item.field_value_decoder(codec, reflection, message, field, 0);
}

void LuaProtobufCodec::decode_repeated(LuaProtobufCodec* codec, const google::protobuf::Reflection* reflection, const google::protobuf::Message* message, const google::protobuf::FieldDescriptor *field) {
    const char *field_name = field->name().c_str();
    google::protobuf::FieldDescriptor::Type type = field->type();
    int size = reflection->FieldSize(*message, field);

    lua_pushstring(codec->L, field_name);
    lua_newtable(codec->L);
    lua_rawset(codec->L, -3);
    lua_pushstring(codec->L, field_name);
    lua_rawget(codec->L, -2);


    for (int i = 0; i < size; ++i) {
        LuaProtobufMappingItem& item = codec->protobuf_lua_mapping[type];
        item.repeated_value_decoder(codec, reflection, message, field, i);
    }
    lua_pop(codec->L, 1);
}

DEFINE_ENCODER(field, TYPE_DOUBLE) {
    double val = lua_tonumber(codec->L, -1);
    reflection->SetDouble(message, field, val);
}

DEFINE_ENCODER(field, TYPE_FLOAT) {
    float val = static_cast<float>(lua_tonumber(codec->L, -1));
    reflection->SetFloat(message, field, val);
}

DEFINE_ENCODER(field, TYPE_INT64) {
#if (LUA_VERSION_NUM >= 503)
    google::protobuf::int64 val = lua_tointeger(codec->L, -1);
#else
    google::protobuf::int64 val = static_cast<google::protobuf::int64>(lua_tonumber(codec->L, -1));
#endif
    reflection->SetInt64(message, field, val);
}

DEFINE_ENCODER(field, TYPE_UINT64) {
#if (LUA_VERSION_NUM >= 503)
    google::protobuf::uint64 val = static_cast<google::protobuf::uint64>(lua_tointeger(codec->L, -1));
#else
    google::protobuf::uint64 val = static_cast<google::protobuf::uint64>(lua_tonumber(codec->L, -1));
#endif
    reflection->SetUInt64(message, field, val);
}

DEFINE_ENCODER(field, TYPE_INT32) {
    google::protobuf::int32 val = static_cast<google::protobuf::int32>(lua_tointeger(codec->L, -1));
    reflection->SetInt32(message, field, val);
}

DEFINE_ENCODER(field, TYPE_FIXED64) {
#if (LUA_VERSION_NUM >= 503)
    google::protobuf::uint64 val = static_cast<google::protobuf::uint64>(lua_tointeger(codec->L, -1));
#else
    google::protobuf::uint64 val = static_cast<google::protobuf::uint64>(lua_tonumber(codec->L, -1));
#endif
    reflection->SetUInt64(message, field, val);
}

DEFINE_ENCODER(field, TYPE_FIXED32) {
    google::protobuf::uint32 val = static_cast<google::protobuf::uint32>(lua_tointeger(codec->L, -1));
    reflection->SetUInt32(message, field, val);
}

DEFINE_ENCODER(field, TYPE_BOOL) {
    bool val = lua_toboolean(codec->L, -1);
    reflection->SetBool(message, field, val);
}

DEFINE_ENCODER(field, TYPE_STRING) {
    size_t len = 0;
    const char *str = lua_tolstring(codec->L, -1, &len);
    std::string data(str, len);
    reflection->SetString(message, field, data);
}

DEFINE_ENCODER(field, TYPE_GROUP) {
}

DEFINE_ENCODER(field, TYPE_MESSAGE) {
    google::protobuf::Message *msg = reflection->MutableMessage(message, field);
    if (msg) {
        encode_message(codec, msg->GetReflection(), msg);
    }
}

DEFINE_ENCODER(field, TYPE_BYTES) {
    std::string scratch;
    reflection->SetString(message, field, *luaL_tobytes(codec->L, -1, &scratch));
}

DEFINE_ENCODER(field, TYPE_UINT32) {
    google::protobuf::uint32 val = static_cast<google::protobuf::uint32>(lua_tointeger(codec->L, -1));
    reflection->SetUInt32(message, field, val);
}

DEFINE_ENCODER(field, TYPE_ENUM) {
    int val = static_cast<int>(lua_tointeger(codec->L, -1));
    const google::protobuf::EnumValueDescriptor *evd = field->enum_type()->FindValueByNumber(val);
    if (!evd) {
        std::string err;
        StringUtils::Format(err, "%s.%s enum value expected, but got %d", message->GetTypeName().c_str(), field->name().c_str(), val);
        codec->AddError(err);
        return;
    }
    reflection->SetEnum(message, field, evd);
}

DEFINE_ENCODER(field, TYPE_SFIXED32) {
    google::protobuf::int32 val = static_cast<google::protobuf::int32>(lua_tointeger(codec->L, -1));
    reflection->SetInt32(message, field, val);
}

DEFINE_ENCODER(field, TYPE_SFIXED64) {
#if (LUA_VERSION_NUM >= 503)
    google::protobuf::int64 val = lua_tointeger(codec->L, -1);
#else
    google::protobuf::int64 val = static_cast<google::protobuf::int64>(lua_tonumber(codec->L, -1));
#endif
    reflection->SetInt64(message, field, val);
}

DEFINE_ENCODER(field, TYPE_SINT32) {
    google::protobuf::int32 val = static_cast<google::protobuf::int32>(lua_tointeger(codec->L, -1));
    reflection->SetInt32(message, field, val);
}

DEFINE_ENCODER(field, TYPE_SINT64) {
#if (LUA_VERSION_NUM >= 503)
    google::protobuf::int64 val = lua_tointeger(codec->L, -1);
#else
    google::protobuf::int64 val = static_cast<google::protobuf::int64>(lua_tonumber(codec->L, -1));
#endif
    reflection->SetInt64(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_DOUBLE) {
    double val = lua_tonumber(codec->L, -1);
    reflection->AddDouble(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_FLOAT) {
    float val = static_cast<float>(lua_tonumber(codec->L, -1));
    reflection->AddFloat(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_INT64) {
#if (LUA_VERSION_NUM >= 503)
    google::protobuf::int64 val = lua_tointeger(codec->L, -1);
#else
    google::protobuf::int64 val = static_cast<google::protobuf::int64>(lua_tonumber(codec->L, -1));
#endif
    reflection->AddInt64(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_UINT64) {
#if (LUA_VERSION_NUM >= 503)
    google::protobuf::uint64 val = static_cast<google::protobuf::uint64>(lua_tointeger(codec->L, -1));
#else
    google::protobuf::uint64 val = static_cast<google::protobuf::uint64>(lua_tonumber(codec->L, -1));
#endif
    reflection->AddUInt64(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_INT32) {
    google::protobuf::int32 val = static_cast<google::protobuf::int32>(lua_tointeger(codec->L, -1));
    reflection->AddInt32(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_FIXED64) {
#if (LUA_VERSION_NUM >= 503)
    google::protobuf::int64 val = lua_tointeger(codec->L, -1);
#else
    google::protobuf::int64 val = static_cast<google::protobuf::int64>(lua_tonumber(codec->L, -1));
#endif
    reflection->AddUInt64(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_FIXED32) {
    google::protobuf::uint32 val = static_cast<google::protobuf::uint32>(lua_tointeger(codec->L, -1));
    reflection->AddUInt32(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_BOOL) {
    bool val = lua_toboolean(codec->L, -1);
    reflection->AddBool(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_STRING) {
    size_t len;
    const char *str = lua_tolstring(codec->L, -1, &len);
    std::string data(str, len);
    reflection->AddString(message, field, data);
}

DEFINE_ENCODER(repeated, TYPE_GROUP) {
    //Deprecated
}

DEFINE_ENCODER(repeated, TYPE_MESSAGE) {
    google::protobuf::Message *msg = reflection->AddMessage(message, field);
    if (msg) {
        encode_message(codec, msg->GetReflection(), msg);
    }
}

DEFINE_ENCODER(repeated, TYPE_BYTES) {
    std::string scratch;
    reflection->AddString(message, field, *luaL_tobytes(codec->L, -1, &scratch));
}

DEFINE_ENCODER(repeated, TYPE_UINT32) {
    google::protobuf::uint32 val = static_cast<google::protobuf::uint32>(lua_tointeger(codec->L, -1));
    reflection->AddUInt32(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_ENUM) {
    int val = static_cast<int>(lua_tointeger(codec->L, -1));
    const google::protobuf::EnumValueDescriptor *evd = field->enum_type()->FindValueByNumber(val);
    if (!evd) {
        std::string err;
        StringUtils::Format(err, "%s.%s enum value expected, but got %d", message->GetTypeName().c_str(), field->name().c_str(), val);
        codec->AddError(err);
        return;
    }
    reflection->AddEnum(message, field, evd);
}

DEFINE_ENCODER(repeated, TYPE_SFIXED32) {
    google::protobuf::int32 val = static_cast<google::protobuf::int32>(lua_tointeger(codec->L, -1));
    reflection->AddInt32(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_SFIXED64) {
#if (LUA_VERSION_NUM >= 503)
    google::protobuf::int64 val = lua_tointeger(codec->L, -1);
#else
    google::protobuf::int64 val = static_cast<google::protobuf::int64>(lua_tonumber(codec->L, -1));
#endif
    reflection->AddInt64(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_SINT32) {
    google::protobuf::int32 val = static_cast<google::protobuf::int32>(lua_tointeger(codec->L, -1));
    reflection->AddInt32(message, field, val);
}

DEFINE_ENCODER(repeated, TYPE_SINT64) {
#if (LUA_VERSION_NUM >= 503)
    google::protobuf::int64 val = lua_tointeger(codec->L, -1);
#else
    google::protobuf::int64 val = static_cast<google::protobuf::int64>(lua_tonumber(codec->L, -1));
#endif
    reflection->AddInt64(message, field, val);
}

DEFINE_DECODER(field, TYPE_DOUBLE) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushnumber(codec->L, reflection->GetDouble(*message, field));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(field, TYPE_FLOAT) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushnumber(codec->L, reflection->GetFloat(*message, field));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(field, TYPE_INT64) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushinteger(codec->L, static_cast<lua_Integer>(reflection->GetInt64(*message, field)));
    lua_rawset(codec->L, -3);
}
DEFINE_DECODER(field, TYPE_UINT64) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushinteger(codec->L, static_cast<lua_Integer>(reflection->GetUInt64(*message, field)));
    lua_rawset(codec->L, -3);
}
DEFINE_DECODER(field, TYPE_INT32) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushinteger(codec->L, reflection->GetInt32(*message, field));
    lua_rawset(codec->L, -3);
}
DEFINE_DECODER(field, TYPE_FIXED64) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushinteger(codec->L, static_cast<lua_Integer>(reflection->GetUInt64(*message, field)));
    lua_rawset(codec->L, -3);
}
DEFINE_DECODER(field, TYPE_FIXED32) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushinteger(codec->L, reflection->GetUInt32(*message, field));
    lua_rawset(codec->L, -3);
}
DEFINE_DECODER(field, TYPE_BOOL) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushboolean(codec->L, reflection->GetBool(*message, field));
    lua_rawset(codec->L, -3);
}
DEFINE_DECODER(field, TYPE_STRING) {
    std::string scratch;
    const std::string& value = reflection->GetStringReference(*message, field, &scratch);
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushlstring(codec->L, value.data(), value.length());
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(field, TYPE_GROUP) {

}

DEFINE_DECODER(field, TYPE_MESSAGE) {
    const google::protobuf::Message* msg = ReflectionGetMessage(reflection, message, field);
    const google::protobuf::Descriptor *descriptor = msg->GetDescriptor();
    const char *field_name = field->name().c_str();
    lua_pushstring(codec->L, field_name);
    lua_newtable(codec->L);
    lua_rawset(codec->L, -3);
    lua_pushstring(codec->L, field_name);
    lua_rawget(codec->L, -2);

    int field_count = descriptor->field_count();
    for (int i = 0; i < field_count; ++i) {
        const google::protobuf::FieldDescriptor *field = descriptor->field(i);
        decode_field(codec, msg->GetReflection(), msg, field);
    }
    lua_pop(codec->L, 1);
}

DEFINE_DECODER(field, TYPE_BYTES) {
    std::string scratch;
    const std::string& value = reflection->GetStringReference(*message, field, &scratch);
    lua_pushstring(codec->L, field->name().c_str());
    if (codec->get_opts().bytes_as_string)
        lua_pushlstring(codec->L, value.data(), value.length());
    else
        lua_pushbytes(codec->L, &value);
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(field, TYPE_UINT32) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushinteger(codec->L, static_cast<lua_Integer>(reflection->GetUInt32(*message, field)));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(field, TYPE_ENUM) {
    const google::protobuf::EnumValueDescriptor *evd = reflection->GetEnum(*message, field);
    if (!evd) {
        return;
    }
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushinteger(codec->L, evd->number());
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(field, TYPE_SFIXED32) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushinteger(codec->L, reflection->GetInt32(*message, field));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(field, TYPE_SFIXED64) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushinteger(codec->L, reflection->GetInt64(*message, field));
    lua_rawset(codec->L, -3);

}

DEFINE_DECODER(field, TYPE_SINT32) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushinteger(codec->L, reflection->GetInt32(*message, field));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(field, TYPE_SINT64) {
    lua_pushstring(codec->L, field->name().c_str());
    lua_pushinteger(codec->L, reflection->GetInt64(*message, field));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_DOUBLE) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushnumber(codec->L, reflection->GetRepeatedDouble(*message, field, index));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_FLOAT) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushnumber(codec->L, reflection->GetRepeatedFloat(*message, field, index));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_INT64) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushinteger(codec->L, static_cast<lua_Integer>(reflection->GetRepeatedInt64(*message, field, index)));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_UINT64) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushinteger(codec->L, static_cast<lua_Integer>(reflection->GetRepeatedUInt64(*message, field, index)));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_INT32) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushinteger(codec->L, reflection->GetRepeatedInt32(*message, field, index));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_FIXED64) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushinteger(codec->L, static_cast<lua_Integer>(reflection->GetRepeatedUInt64(*message, field, index)));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_FIXED32) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushinteger(codec->L, static_cast<lua_Integer>(reflection->GetRepeatedUInt32(*message, field, index)));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_BOOL) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushboolean(codec->L, reflection->GetRepeatedBool(*message, field, index));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_STRING) {
    std::string scratch;
    const std::string& value = reflection->GetRepeatedStringReference(*message, field, index, &scratch);
    lua_pushinteger(codec->L, index + 1);
    lua_pushlstring(codec->L, value.data(), value.length());
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_GROUP) {
}

DEFINE_DECODER(repeated, TYPE_MESSAGE) {
    const google::protobuf::Message* msg = &(reflection->GetRepeatedMessage(*message, field, index));
    const google::protobuf::Descriptor *descriptor = msg->GetDescriptor();
    lua_pushinteger(codec->L, index + 1);
    lua_newtable(codec->L);
    lua_rawset(codec->L, -3);
    lua_pushinteger(codec->L, index + 1);
    lua_rawget(codec->L, -2);

    int field_count = descriptor->field_count();
    for (int i = 0; i < field_count; ++i) {
        const google::protobuf::FieldDescriptor *field = descriptor->field(i);
        decode_field(codec, msg->GetReflection(), msg, field);
    }
    lua_pop(codec->L, 1);
}

DEFINE_DECODER(repeated, TYPE_BYTES) {
    std::string scratch;
    const std::string& value = reflection->GetRepeatedStringReference(*message, field, index, &scratch);
    lua_pushinteger(codec->L, index + 1);
    if (codec->get_opts().bytes_as_string)
        lua_pushlstring(codec->L, value.data(), value.length());
    else
        lua_pushbytes(codec->L, &value);
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_UINT32) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushinteger(codec->L, reflection->GetRepeatedUInt32(*message, field, index));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_ENUM) {
    const google::protobuf::EnumValueDescriptor *evd = reflection->GetRepeatedEnum(*message, field, index);
    if (!evd) {
        return;
    }

    lua_pushinteger(codec->L, index + 1);
    lua_pushinteger(codec->L, evd->number());
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_SFIXED32) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushinteger(codec->L, reflection->GetRepeatedInt32(*message, field, index));
    lua_rawset(codec->L, -3);
}
DEFINE_DECODER(repeated, TYPE_SFIXED64) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushinteger(codec->L, static_cast<lua_Integer>(reflection->GetRepeatedInt64(*message, field, index)));
    lua_rawset(codec->L, -3);
}
DEFINE_DECODER(repeated, TYPE_SINT32) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushinteger(codec->L, reflection->GetRepeatedInt32(*message, field, index));
    lua_rawset(codec->L, -3);
}

DEFINE_DECODER(repeated, TYPE_SINT64) {
    lua_pushinteger(codec->L, index + 1);
    lua_pushinteger(codec->L, static_cast<lua_Integer>(reflection->GetRepeatedInt64(*message, field, index)));
    lua_rawset(codec->L, -3);
}

#define CODEC_FUNC(TYPE) ENCODER_FUNC(field,TYPE), ENCODER_FUNC(repeated,TYPE), DECODER_FUNC(field,TYPE), DECODER_FUNC(repeated,TYPE)

LuaProtobufCodec::LuaProtobufMappingItem LuaProtobufCodec::protobuf_lua_mapping[google::protobuf::FieldDescriptor::MAX_TYPE+1] = {
    {0, 0, 0, 0, 0, 0},
    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_DOUBLE)},
    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_FLOAT)},
    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_INT64)},

    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_UINT64)},
    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_INT32)},

    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_FIXED64)},
    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_FIXED32)},
    {LUA_TBOOLEAN,	LUA_TBOOLEAN_MASK | LUA_TNUMBER_MASK, CODEC_FUNC(TYPE_BOOL)},
    {LUA_TSTRING,	LUA_TNUMBER | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_STRING)},
    {LUA_TNIL,		LUA_TNIL_MASK, CODEC_FUNC(TYPE_GROUP)},
    {LUA_TTABLE,	LUA_TTABLE_MASK, CODEC_FUNC(TYPE_MESSAGE)},

    {LUA_TSTRING,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK |LUA_TUSERDATA_MASK, CODEC_FUNC(TYPE_BYTES)},
    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_UINT32)},
    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_ENUM)},
    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_SFIXED32)},
    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_SFIXED64)},
    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_SINT32)},
    {LUA_TNUMBER,	LUA_TNUMBER_MASK | LUA_TSTRING_MASK, CODEC_FUNC(TYPE_SINT64)},
};

//Protobuf lua api

static int pb_clear(lua_State *L) {
    tinynet::rpc::ProtobufManager* pb =  luaL_checkpb(L, lua_upvalueindex(1));
    pb->Clear();
    return 0;
}

static int pb_mapping(lua_State *L) {
    tinynet::rpc::ProtobufManager* pb = luaL_checkpb(L, lua_upvalueindex(1));
    const char *virtual_path = luaL_checkstring(L, 1);
    const char *disk_path = luaL_checkstring(L, 2);
    pb->MapPath(virtual_path, disk_path);
    return 0;
}

static int pb_import(lua_State *L) {
    tinynet::rpc::ProtobufManager* pb = luaL_checkpb(L, lua_upvalueindex(1));
    const char *filename = luaL_checkstring(L, 1);
    bool result = pb->Import(filename);
    if (!result) {
        return luaL_error(L, pb->get_compile_errors().c_str());
    }
    return 0;
}

static int pb_tostring(lua_State *L) {
    tinynet::rpc::ProtobufManager* pb = luaL_checkpb(L, lua_upvalueindex(1));
    size_t len = 0;
    const char *type_name = luaL_checkstring(L, 1);
    const char *bin = luaL_checklstring(L, 2, &len);
    std::unique_ptr<google::protobuf::Message> message(pb->CreateMessage(type_name));
    if (!message) {
        return luaL_error(L, "pb_tostring can not find message %s!", type_name);
    }
    bool result = message->ParseFromArray(bin, static_cast<int>(len));
    if (!result) {
        return luaL_error(L, "pb_tostring %s message->ParseFromArray failed!", type_name);
    }

    const std::string &msg = message->Utf8DebugString();
    lua_pushlstring(L, msg.c_str(), msg.length());
    return 1;
}


static int pb_encode(lua_State *L) {
    tinynet::rpc::ProtobufManager* pb = luaL_checkpb(L, lua_upvalueindex(1));
    tinynet::lua::ProtobufCodecOptions opts;
    const char *type_name = luaL_checkstring(L, 1);
    luaL_argcheck(L, lua_istable(L, 2), 2, "message body is invalid, table expected.");
    int top = lua_gettop(L);
    if (top == 3) {
        luaL_argcheck(L, lua_istable(L, 3), 3, "table expected");
        LuaState S{ L };
        S >> opts;
        lua_pushvalue(L, 2);
    }
    std::unique_ptr<google::protobuf::Message> message(pb->CreateMessage(type_name));
    if (!message) {
        return luaL_error(L, "can not find message by name:%s.", type_name);
    }
    LuaProtobufCodec codec(L, pb, opts);
    if (codec.Encode(message.get()) > 0) {
        return luaL_error(L, codec.get_errors().c_str());
    }
    lua_settop(L, top);
    std::string bin;
    if (!message->SerializeToString(&bin)) {
        return luaL_error(L, "message %s SerializeToString failed.", type_name);
    }
    if (opts.encode_as_bytes)
        lua_pushandswapbytes(L, &bin);
    else
        lua_pushlstring(L, bin.data(), bin.size());
    return 1;
}

static int pb_decode(lua_State *L) {
    tinynet::rpc::ProtobufManager* pb = luaL_checkpb(L, lua_upvalueindex(1));
    tinynet::lua::ProtobufCodecOptions opts;
    const char *type_name = luaL_checkstring(L, 1);
    int type = lua_type(L, 2);
    luaL_argcheck(L, 2, type == LUA_TSTRING || type == LUA_TUSERDATA, "string or bytes expected");
    int top = lua_gettop(L);
    if (top == 3) {
        luaL_argcheck(L, lua_istable(L, 3), 3, "table expected");
        LuaState S{ L };
        S >> opts;
    }
    std::unique_ptr<google::protobuf::Message> message(pb->CreateMessage(type_name));
    if (!message) {
        return luaL_error(L, "can not find message by name:%s", type_name);
    }
    switch (type) {
    case LUA_TSTRING: {
        size_t len = 0;
        const char *bin = luaL_checklstring(L, 2, &len);
        if (!message->ParseFromArray(bin, (int)len)) {
            return luaL_error(L, "message %s ParseFromArray failed.", type_name);
        }
        break;
    }
    case LUA_TUSERDATA: {
        auto bin = luaL_checkbytes(L, 2);
        if (!message->ParseFromString(*bin)) {
            return luaL_error(L, "message %s ParseFromZeroCopyStream failed.", type_name);
        }
        break;
    }
    default:
        return luaL_argerror(L, 2, "string or bytes expected");
    }
    LuaProtobufCodec codec(L, pb, opts);
    if (codec.Decode(message.get()) > 0) {
        return luaL_error(L, codec.get_errors().c_str());
    }
    return 1;
}

static int lua_pb_new(lua_State *L) {
    auto app = lua_getapp(L);
    void* ud = lua_newuserdata(L, sizeof(tinynet::rpc::ProtobufManager));
    auto pb = new(ud)tinynet::rpc::ProtobufManager(app);
    pb->Init();
    luaL_getmetatable(L, PROTOBUF_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_pb_delete(lua_State *L) {
    auto pb = luaL_checkpb(L, 1);
    pb->~ProtobufManager();
    return 0;
}

static const struct luaL_Reg meta_methods[] = {
    { "__gc", lua_pb_delete },
    { 0, 0 }
};

static const struct luaL_Reg methods[] = {
    { "clear", pb_clear },
    { "mapping", pb_mapping },
    { "import", pb_import },
    { "tostring", pb_tostring },
    { "encode", pb_encode },
    { "decode", pb_decode },
    { 0, 0 }
};

int luaopen_pb(lua_State* L) {
    luaL_newmetatable(L, PROTOBUF_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta_methods, 0);
    lua_pop(L, 1);

    lua_newtable(L);
    lua_pb_new(L);
    luaL_setfuncs(L, methods, 1);
    return 1;
}
