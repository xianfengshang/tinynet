// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_rapidjson.h"
#include "lua_types.h"
#include "lua_common_types.h"
#include "lua_proto_types.h"
#include "base/io_buffer.h"
#include "base/string_view.h"
#include "base/json_types.h"
#include "lua_bytes.h"
#include "lua_compat.h"
#include <limits>
#include <unordered_set>

namespace json {
namespace internal {

class StringAsBuffer {
  public:
    explicit StringAsBuffer(std::string& buf):
        idx_(0),
        len_(buf.size()),
        buf_(buf) {
    }
  public:
    typedef char Ch;
  public:
    Ch Peek() { return idx_ >= len_ ? '\0' : buf_[idx_]; }

    Ch Take() { return buf_[idx_++]; }

    size_t Tell() { return idx_; }

    Ch* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
    void Put(Ch c) { buf_.push_back(c); }
    void Flush() { }
    size_t PutEnd(Ch*) { RAPIDJSON_ASSERT(false); return 0; }
  private:
    size_t idx_;
    size_t len_;
    std::string& buf_;
};


#define DECODE_TYPE_OBJECT -1
#define DECODE_TYPE_ARRAY 1

class Decoder {
  public:
    Decoder(lua_State* L, const tinynet::lua::JsonCodecOptions& opts) :L_(L), opts_(opts) {
        stack_.reserve(16);
    }
  public:
    bool Null() {
        lua_pushlightuserdata(L_, NULL);
        commit();
        return true;
    }
    bool Bool(bool b) {
        lua_pushboolean(L_, b);
        commit();
        return true;
    }
    bool Int(int i) {
        lua_pushinteger(L_, i);
        commit();
        return true;
    }
    bool Uint(unsigned i) {
        lua_pushinteger(L_, static_cast<lua_Integer>(i));
        commit();
        return true;
    }
    bool Int64(int64_t i) {
        lua_pushinteger(L_, i);
        commit();
        return true;
    }

    bool Uint64(uint64_t i) {
        lua_pushinteger(L_, static_cast<lua_Integer>(i));
        commit();
        return true;
    }

    bool Double(double d) {
        lua_pushnumber(L_, d);
        commit();
        return true;
    }

    bool RawNumber(const char* str, rapidjson::SizeType length, bool copy) {
        lua_pushlstring(L_, str, length);
        commit();
        return true;
    }
    bool String(const char* str, rapidjson::SizeType length, bool copy) {
        lua_pushlstring(L_, str, length);
        commit();
        return true;
    }

    bool StartObject() {
        if (opts_.decode_max_depth > 0 && (int)stack_.size() > opts_.decode_max_depth) {
            return false;
        }
        lua_newtable(L_);
        stack_.push_back(DECODE_TYPE_OBJECT);
        return true;
    }

    bool Key(const char* str, rapidjson::SizeType length, bool copy) {
        lua_pushlstring(L_, str, length);
        return true;
    }

    bool EndObject(rapidjson::SizeType memberCount) {
        stack_.pop_back();
        commit();
        return true;
    }

    bool StartArray() {
        if (opts_.decode_max_depth > 0 && (int)stack_.size() > opts_.decode_max_depth) {
            return false;
        }
        lua_newtable(L_);
        stack_.push_back(DECODE_TYPE_ARRAY);
        return true;
    }

    bool EndArray(rapidjson::SizeType elementCount) {
        stack_.pop_back();
        commit();
        return true;
    }

    void decode_value(const rapidjson::Value& value) {
        auto type = value.GetType();
        switch (type) {
        case rapidjson::kNullType: {
            lua_pushlightuserdata(L_, NULL);
            break;
        }
        case rapidjson::kFalseType:
        case rapidjson::kTrueType: {
            lua_pushboolean(L_, value.GetBool());
            break;
        }
        case rapidjson::kObjectType: {
            lua_newtable(L_);
            for (auto& entry : value.GetObj()) {
                lua_pushlstring(L_, entry.name.GetString(), entry.name.GetStringLength());
                decode_value(entry.value);
                lua_rawset(L_, -3);
            }
            break;
        }
        case rapidjson::kArrayType: {
            lua_newtable(L_);
            int index = 0;
            for (auto& entry : value.GetArray()) {
                decode_value(entry);
                lua_rawseti(L_, -2, ++index);
            }
            break;
        }
        case rapidjson::kStringType: {
            lua_pushlstring(L_, value.GetString(), value.GetStringLength());
            break;
        }
        case rapidjson::kNumberType: {
            if (value.IsDouble()) {
                lua_pushnumber(L_, value.GetDouble());
            } else {
                if (value.IsInt()) {
                    lua_pushinteger(L_, value.GetInt());
                } else if(value.IsUint()) {
                    lua_pushinteger(L_, static_cast<lua_Integer>(value.GetUint()));
                } else if (value.IsInt64()) {
#if (LUA_VERSION_NUM >= 503)
                    lua_pushinteger(L_, value.GetInt64());
#else
                    std::string s = std::to_string(value.GetInt64());
                    lua_pushstring(L_, s.c_str());
#endif
                } else if (value.IsUint64()) {
#if (LUA_VERSION_NUM >= 503)
                    lua_pushinteger(L_, static_cast<lua_Integer>(value.GetUint64()));
#else
                    std::string s = std::to_string(value.GetUint64());
                    lua_pushstring(L_, s.c_str());
#endif
                }
            }
            break;
        }
        default:
            break;
        }
    }
  private:
    void commit() {
        if (stack_.empty()) return;

        if (stack_.back() == DECODE_TYPE_OBJECT) {
            lua_rawset(L_, -3);
        } else {
            lua_rawseti(L_, -2, stack_.back()++);
        }
    }
  private:
    lua_State* L_;
    std::vector<int> stack_;
    tinynet::lua::JsonCodecOptions opts_;
};

class Encoder {
  public:
    Encoder(lua_State *L, const tinynet::lua::JsonCodecOptions& opts) : L_(L),opts_(opts) {
    }
  private:
    template<typename Writer>
    void encode_value(Writer& writer, int depth) {
        int type = lua_type(L_, -1);
        switch (type) {
        case LUA_TNIL: {
            writer.Null();
            break;
        }
        case LUA_TBOOLEAN: {
            writer.Bool(lua_toboolean(L_, -1) != 0);
            break;
        }
        case LUA_TLIGHTUSERDATA: {
            auto ud = lua_touserdata(L_, -1);
            if (ud == NULL) {
                writer.Null();
            } else {
                std::string value;
                StringUtils::Format(value, "lightuserdata %p", ud);
                writer.String(value);
            }
            break;
        }
        case LUA_TNUMBER: {
            if (lua_isinteger(L_, -1)) {
                writer.Int64(lua_tointeger(L_, -1));
            } else {
                writer.Double(lua_tonumber(L_, -1));
            }
            break;
        }
        case LUA_TSTRING: {
            size_t len;
            const char* value = lua_tolstring(L_, -1, &len);
            writer.String(value, static_cast<rapidjson::SizeType>(len));
            break;
        }
        case LUA_TTABLE: {
            ++depth;
            if (opts_.encode_max_depth > 0 && depth >= opts_.encode_max_depth) {
                std::string err;
                StringUtils::Format(err, "Cannot serialise, excessive nesting(%d)", depth);
                errors_.emplace_back(std::move(err));
                writer.String(err);
                return;
            }
            auto ref = lua_topointer(L_, -1);
            if (refs_.find(ref) != refs_.end()) {
                std::string value;
                StringUtils::Format(value, "table %p", ref);
                writer.String(value);
                return;
            }
            refs_.insert(ref);
            if (lua_isarray(L_, -1)) {
#if (LUA_VERSION_NUM >= 503)
                size_t len = lua_rawlen(L_, -1);
#else
                size_t len = lua_objlen(L_, -1);
#endif
                encode_array(writer, static_cast<int>(len), depth);
            } else {
                encode_object(writer, depth);
            }
            break;
        }
        case LUA_TFUNCTION: {
            auto ref = lua_topointer(L_, -1);
            std::string value;
            StringUtils::Format(value, "function %p", ref);
            writer.String(value);
            break;
        }
        default: {
            std::string err;
            StringUtils::Format(err, "unsupported type: %s", lua_typename(L_, type));
            errors_.emplace_back(std::move(err));
            writer.String(err);
            return;
        }
        }
    }

    template<typename Writer>
    void encode_object(Writer& writer, int depth) {
        lua_pushnil(L_);
        if (!lua_next(L_, -2)) { //Empty table
            if (opts_.empty_table_as_array) {
                writer.StartArray();
                writer.EndArray();
            } else {
                writer.StartObject();
                writer.EndObject();
            }
            return;
        }

        size_t len;
        const char* key;
        writer.StartObject();
        if (opts_.order_map_entries) {
            std::vector<tinynet::string_view> keys;
            do {
                if (lua_isstring(L_, -2)) {
                    key = lua_tolstring(L_, -2, &len);
                    keys.emplace_back(key, static_cast<int>(len));
                }
                lua_pop(L_, 1);
            } while (lua_next(L_, -2));

            std::sort(keys.begin(), keys.end());
            for (auto& key : keys) {
                writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.size()));
                lua_pushlstring(L_, key.data(), key.size());
                lua_rawget(L_, -2);
                encode_value(writer, depth);
                lua_pop(L_, 1);
            }
        } else {
            do {
                int type = lua_type(L_, -2);
                if (type == LUA_TSTRING || type == LUA_TNUMBER) {
                    if (type == LUA_TSTRING) {
                        key = lua_tolstring(L_, -2, &len);
                        writer.Key(key, static_cast<rapidjson::SizeType>(len));
                    } else if (type == LUA_TNUMBER) {
                        std::string str;
                        if (lua_isinteger(L_, -2)) {
                            str = std::to_string(lua_tointeger(L_, -2));
                        } else {
                            str = std::to_string(lua_tonumber(L_, -2));
                        }
                        writer.Key(str.data(), static_cast<rapidjson::SizeType>(str.length()));
                    }
                    encode_value(writer, depth);
                }
                lua_pop(L_, 1);
            } while (lua_next(L_, -2));
        }
        writer.EndObject();
    }

    template<typename Writer>
    void encode_array(Writer& writer, int count, int depth) {
        writer.StartArray();
        for (int i = 0; i < count; ++i) {
            lua_rawgeti(L_, -1, i + 1);
            encode_value(writer, depth);
            lua_pop(L_, 1);
        }
        writer.EndArray();
    }

  public:
    template<typename Stream>
    void encode(Stream& s) noexcept {
        if (opts_.pretty_format) {
            rapidjson::PrettyWriter<Stream> writer(s);
            encode_value(writer, 0);
            return;
        }
        rapidjson::Writer<Stream> writer(s);
        encode_value(writer, 0);
    }
    const std::vector<std::string>& errors() { return errors_; }
  private:
    lua_State* L_;
    tinynet::lua::JsonCodecOptions opts_;
    std::vector<std::string> errors_;
    std::unordered_set<const void*> refs_;
};
}
}

static int json_encode(lua_State *L) {
    tinynet::lua::JsonCodecOptions opts;
    int top = lua_gettop(L);
    if (top == 2) {
        if (!lua_istable(L, 2)) {
            return luaL_argerror(L, 2, "table expected");
        }
        LuaState S{ L };
        S >> opts;
        lua_pushvalue(L, 1);
    } else if(top != 1) {
        return luaL_error(L, "1 or 2 argument[s] expected, but got %d", top);
    }
    std::string s;
    json::internal::StringAsBuffer stream(s);
    json::internal::Encoder encoder(L, opts);
    encoder.encode(stream);
    lua_settop(L, top);
    if (encoder.errors().empty()) {
        if (opts.encode_as_bytes) {
            lua_pushandswapbytes(L, &s);
        } else {
            lua_pushlstring(L, s.data(), s.size());
        }
        return 1;
    }
    const std::string& err = encoder.errors()[0];
    return luaL_error(L, err.c_str());
}

static int json_decode(lua_State *L) {
    tinynet::lua::JsonCodecOptions opts;
    int top = lua_gettop(L);
    if (top == 2) {
        luaL_argcheck(L, lua_istable(L, 2), 2, "table expected");
        LuaState S{ L };
        S >> opts;
    } else if(top != 1) {
        return luaL_error(L, "1 or 2 argument[s] expected, but got %d", top);
    }
    json::internal::Decoder decoder(L, opts);
    rapidjson::Reader reader;
    rapidjson::ParseResult ok;
    int type = lua_type(L, 1);
    switch (type) {
    case LUA_TSTRING: {
        size_t len;
        const char* data = luaL_checklstring(L, 1, &len);
        rapidjson::MemoryStream stream(data, len);
        ok = reader.Parse(stream, decoder);
        break;
    }
    case LUA_TUSERDATA: {
        auto s = luaL_checkbytes(L, 1);
        if (!s) {
            return luaL_argerror(L, 1, "string or bytes expected");
        }
        rapidjson::MemoryStream stream(s->data(), s->size());
        ok = reader.Parse(stream, decoder);
        break;
    }
    default:
#if (LUA_VERSION_NUM >= 503)
        return luaL_typeerror(L, 1, "string or bytes");
#else
        return luaL_typerror(L, 1, "string or bytes");
#endif
    }

    if (ok) {
        return 1;
    }

    lua_settop(L, top);

    return luaL_error(L, "JSON parse error : %s(%u)", rapidjson::GetParseError_En(ok.Code()), ok.Offset());
}

static const luaL_Reg methods[] = {
    {"encode", json_encode},
    {"decode", json_decode },
    {0, 0}
};

LUALIB_API int luaopen_rapidjson(lua_State *L) {
    luaL_newlib(L, methods);
    lua_pushstring(L, "null");
    lua_pushlightuserdata(L, NULL);
    lua_rawset(L, -3);
    return 1;
}
