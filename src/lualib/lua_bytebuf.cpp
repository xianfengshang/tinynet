// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
//ByteBuf wrapper for luaban tool
//See https://github.com/focus-creative-games/luban_examples
#include "lua_bytebuf.h"
#include <memory>
#include "lua_helper.h"
#include "lua_script.h"
#include "lua_compat.h"
#include "lua_proto_types.h"
#include "base/io_buffer.h"
#include "base/io_buffer_stream.h"
#include "app/app_container.h"
#include "tfs/tfs_service.h"

#define  BYTE_BUF_META_TABLE "BYTE_BUF_META_TABLE"

using namespace tinynet;
class LuaByteBuf {
  public:
    LuaByteBuf():
        is_(&buf_),
        os_(&buf_) {
    }
  public:
    bool readBool(bool& out) {
        return is_.read(&out, sizeof(out)) == sizeof(out);
    }
    bool readByte(uint8_t& out) {
        return is_.read(&out, sizeof(out)) == sizeof(out);
    }
    bool readFshort(int16_t& out) {
        return is_.read(&out, sizeof(out)) == sizeof(out);
    }
    bool readFint(int32_t& out) {
        return is_.read(&out, sizeof(out)) == sizeof(out);
    }
    bool readFlong(int64_t& out) {
        return is_.read(&out, sizeof(out)) == sizeof(out);
    }
    bool readSize(int& out) {
        uint32_t re;
        if (readUint(re)) {
            out = re;
            return true;
        } else {
            return false;
        }
    }
    bool readShort(int16_t& out) {
        uint8_t data[3];
        size_t len = is_.peek(data, sizeof(data));
        if (len == 0) return false;
        size_t beginPos = 0;
        int32_t h = (data[beginPos] & 0xff);
        if (h < 0x80) {
            beginPos++;
            out = (int16_t)h;
        } else if (h < 0xc0) {
            if (len < 2) return false;
            int32_t x = ((h & 0x3f) << 8) | (data[beginPos + 1] & 0xff);
            beginPos += 2;
            out = (int16_t)x;
        } else if (h == 0xff) {
            if (len < 3) return false;
            int32_t x = ((data[beginPos + 1] & 0xff) << 8) | (data[beginPos + 2] & 0xff);
            beginPos += 3;
            out = (int16_t)x;
        } else {
            return false;
        }
        is_.seekg(is_.tellg() + beginPos);
        return true;
    }
    bool readInt(int32_t& out) {
        return readUint(*(uint32_t*)&out);
    }

    bool readUint(uint32_t& out) {
        uint8_t data[5];
        size_t len = is_.peek(data, sizeof(data));
        if (len == 0) return false;
        size_t beginPos = 0;
        uint32_t h = data[beginPos];
        if (h < 0x80) {
            beginPos++;
            out = h;
        } else if (h < 0xc0) {
            if (len < 2) return false;
            uint32_t x = ((h & 0x3f) << 8) | data[beginPos + 1];
            beginPos += 2;
            out = x;
        } else if (h < 0xe0) {
            if (len < 3) return false;
            uint32_t x = ((h & 0x1f) << 16) | ((uint32_t)data[beginPos + 1] << 8) | data[beginPos + 2];
            beginPos += 3;
            out = x;
        } else if (h < 0xf0) {
            if (len < 4) return false;
            uint32_t x = ((h & 0x0f) << 24) | ((uint32_t)data[beginPos + 1] << 16) | ((uint32_t)data[beginPos + 2] << 8) | data[beginPos + 3];
            beginPos += 4;
            out = x;
        } else {
            if (len < 5) return false;
            uint32_t x = ((uint32_t)data[beginPos + 1] << 24) | ((uint32_t)(data[beginPos + 2] << 16))
                         | ((uint32_t)data[beginPos + 3] << 8) | ((uint32_t)data[beginPos + 4]);
            beginPos += 5;
            out = x;
        }
        is_.seekg(is_.tellg() + beginPos);
        return true;
    }

    bool readLong(int64_t& out) {
        return readUlong((uint64_t&)*(uint64_t*)&out);
    }

    bool readUlong(uint64_t& out) {
        uint8_t data[9];
        size_t len = is_.peek(data, sizeof(data));
        if (len == 0) return false;
        size_t beginPos = 0;
        uint32_t h = data[beginPos];
        if (h < 0x80) {
            beginPos++;
            out = h;
        } else if (h < 0xc0) {
            if (len < 2) return false;
            uint32_t x = ((h & 0x3f) << 8) | data[beginPos + 1];
            beginPos += 2;
            out = x;
        } else if (h < 0xe0) {
            if (len < 3) return false;
            uint32_t x = ((h & 0x1f) << 16) | ((uint32_t)data[beginPos + 1] << 8) | data[beginPos + 2];
            beginPos += 3;
            out = x;
        } else if (h < 0xf0) {
            if (len < 4) return false;
            uint32_t x = ((h & 0x0f) << 24) | ((uint32_t)data[beginPos + 1] << 16) | ((uint32_t)data[beginPos + 2] << 8) | data[beginPos + 3];
            beginPos += 4;
            out = x;
        } else if (h < 0xf8) {
            if (len < 5) return false;
            uint32_t xl = ((uint32_t)data[beginPos + 1] << 24) | ((uint32_t)(data[beginPos + 2] << 16)) | ((uint32_t)data[beginPos + 3] << 8) | (data[beginPos + 4]);
            uint32_t xh = h & 0x07;
            beginPos += 5;
            out = ((uint64_t)xh << 32) | xl;
        } else if (h < 0xfc) {
            if (len < 6) return false;
            uint32_t xl = ((uint32_t)data[beginPos + 2] << 24) | ((uint32_t)(data[beginPos + 3] << 16)) | ((uint32_t)data[beginPos + 4] << 8) | (data[beginPos + 5]);
            uint32_t xh = ((h & 0x03) << 8) | data[beginPos + 1];
            beginPos += 6;
            out = ((uint64_t)xh << 32) | xl;
        } else if (h < 0xfe) {
            if (len < 7) return false;
            uint32_t xl = ((uint32_t)data[beginPos + 3] << 24) | ((uint32_t)(data[beginPos + 4] << 16)) | ((uint32_t)data[beginPos + 5] << 8) | (data[beginPos + 6]);
            uint32_t xh = ((h & 0x01) << 16) | ((uint32_t)data[beginPos + 1] << 8) | data[beginPos + 2];
            beginPos += 7;
            out = ((uint64_t)xh << 32) | xl;
        } else if (h < 0xff) {
            if (len < 8) return false;
            uint32_t xl = ((uint32_t)data[beginPos + 4] << 24) | ((uint32_t)(data[beginPos + 5] << 16)) | ((uint32_t)data[beginPos + 6] << 8) | (data[beginPos + 7]);
            uint32_t xh = /*((h & 0x01) << 24) |*/ ((uint32_t)data[beginPos + 1] << 16) | ((uint32_t)data[beginPos + 2] << 8) | data[beginPos + 3];
            beginPos += 8;
            out = ((uint64_t)xh << 32) | xl;
        } else {
            if (len < 9) return false;
            uint32_t xl = ((uint32_t)data[beginPos + 5] << 24) | ((uint32_t)(data[beginPos + 6] << 16)) | ((uint32_t)data[beginPos + 7] << 8) | (data[beginPos + 8]);
            uint32_t xh = ((uint32_t)data[beginPos + 1] << 24) | ((uint32_t)data[beginPos + 2] << 16) | ((uint32_t)data[beginPos + 3] << 8) | data[beginPos + 4];
            beginPos += 9;
            out = ((uint64_t)xh << 32) | xl;
        }
        is_.seekg(is_.tellg() + beginPos);
        return true;
    }

    bool readSlong(int64_t& out) {
        uint64_t x;
        if (readUlong(x)) {
            out = ((int64_t)(x >> 1) ^ -((int64_t)x & 1));
            return true;
        } else {
            return false;
        }
    }

    bool readFloat(float& out) {
        uint8_t data[4];
        size_t len = is_.peek(data, sizeof(data));
        if (len != 4) return false;
        size_t beginPos = 0;
        uint8_t* b = &data[beginPos];
        out = *reinterpret_cast<float*>(b);
        beginPos += 4;
        is_.seekg(is_.tellg() + beginPos);
        return true;
    }

    bool readDouble(double& out) {
        uint8_t data[8];
        size_t len = is_.peek(data, sizeof(data));
        if (len != 8) return false;
        size_t beginPos = 0;
        uint8_t* b = &data[beginPos];
        out = *reinterpret_cast<double*>(b);
        beginPos += 8;
        is_.seekg(is_.tellg() + beginPos);
        return true;
    }

    bool readString(std::string& x) {
        auto pos = is_.tellg();
        int n;
        if (!readSize(n)) return false;
        if (n == 0) return true;
        std::string s(n, 0);
        if (is_.read(&s[0], n) != (size_t)n) {
            is_.seekg(pos);
            return false;
        }
        x.swap(s);
        return true;
    }

    void appendBuffer(const char* buf, int len) {
        os_.write(buf, len);
    }

    void clear() {
        is_.seekg(0);
        os_.seekp(0);
        buf_.clear();
    }

    std::string toString() {
        return buf_.tostring();
    }

    IOBuffer& mutable_buf() { return buf_; }
  private:
    IOBuffer buf_;
    IBufferStream is_;
    OBufferStream os_;
};

static LuaByteBuf* luaL_checkbytebuf(lua_State *L, int idx) {
    return (LuaByteBuf*)luaL_checkudata(L, idx, BYTE_BUF_META_TABLE);
}

static int bytebuf_readBool(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    bool value;
    if (!bytebuf->readBool(value)) {
        return luaL_error(L, "readBool failed");
    }
    lua_pushboolean(L, value);
    return 1;
}

static int bytebuf_readByte(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    uint8_t value;
    if (!bytebuf->readByte(value)) {
        return luaL_error(L, "readByte failed");
    }
    lua_pushinteger(L, value);
    return 1;
}

static int bytebuf_readShort(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    int16_t value;
    if (!bytebuf->readShort(value)) {
        return luaL_error(L, "readShort failed");
    }
    lua_pushinteger(L, value);
    return 1;
}

static int bytebuf_readFshort(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    int16_t value;
    if (!bytebuf->readFshort(value)) {
        return luaL_error(L, "readFshort failed");
    }
    lua_pushinteger(L, value);
    return 1;
}

static int bytebuf_readInt(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    int32_t value;
    if (!bytebuf->readInt(value)) {
        return luaL_error(L, "readInt failed");
    }
    lua_pushinteger(L, value);
    return 1;
}

static int bytebuf_readFint(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    int32_t value;
    if (!bytebuf->readFint(value)) {
        return luaL_error(L, "readFint failed");
    }
    lua_pushinteger(L, value);
    return 1;
}

static int bytebuf_readLong(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    int64_t value;
    if (!bytebuf->readLong(value)) {
        return luaL_error(L, "readLong failed");
    }
    lua_pushinteger(L, value);
    return 1;
}

static int bytebuf_readFlong(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    int64_t value;
    if (!bytebuf->readFlong(value)) {
        return luaL_error(L, "readFlong failed");
    }
    lua_pushinteger(L, value);
    return 1;
}

static int bytebuf_readFloat(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    float value;
    if (!bytebuf->readFloat(value)) {
        return luaL_error(L, "readFloat failed");
    }
    lua_pushnumber(L, value);
    return 1;
}

static int bytebuf_readDouble(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    double value;
    if (!bytebuf->readDouble(value)) {
        return luaL_error(L, "readDouble failed");
    }
    lua_pushnumber(L, value);
    return 1;
}

static int bytebuf_readSize(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    int value;
    if (!bytebuf->readSize(value)) {
        return luaL_error(L, "readSize failed");
    }
    lua_pushinteger(L, value);
    return 1;
}

static int bytebuf_readString(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    std::string value;
    if (!bytebuf->readString(value)) {
        return luaL_error(L, "readString failed");
    }
    lua_pushlstring(L, value.data(), value.length());
    return 1;
}

static int bytebuf_clear(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    bytebuf->clear();
    return 0;
}
static int bytebuf_appendBuffer(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    size_t len;
    const char* buffer = luaL_checklstring(L, 2, &len);
    bytebuf->appendBuffer(buffer, (int)len);
    return 0;
}

static int bytebuf_loadFromFile(lua_State *L) {
    auto app = lua_getapp(L);
    auto bytebuf = luaL_checkbytebuf(L, 1);
    const char* path = luaL_checkstring(L, 2);
    if (!app->get<tinynet::tfs::TfsService>()->Exists(path)) {
        return luaL_error(L, "File %s not exists", path);
    }
    bytebuf->clear();
    bool res = app->get<tinynet::tfs::TfsService>()->LoadFile(path, &bytebuf->mutable_buf());
    if (!res) {
        return luaL_error(L, "Load file %s error", path);
    }
    return 0;
}

static int bytebuf_new(lua_State *L) {
    auto ptr = lua_newuserdata(L, sizeof(LuaByteBuf));
    (void)new(ptr) LuaByteBuf();
    luaL_getmetatable(L, BYTE_BUF_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int bytebuf_delete(lua_State *L) {
    auto bytebuf = luaL_checkbytebuf(L, 1);
    bytebuf->~LuaByteBuf();
    return 0;
}

static const luaL_Reg meta_methods[] = {
    { "readBool", bytebuf_readBool },
    { "readByte", bytebuf_readByte },
    { "readShort", bytebuf_readShort },
    { "readFshort", bytebuf_readFshort },
    { "readInt", bytebuf_readInt },
    { "readFint", bytebuf_readFint },
    { "readLong", bytebuf_readLong },
    { "readFlong", bytebuf_readFlong },
    { "readFloat", bytebuf_readFloat },
    { "readDouble", bytebuf_readDouble },
    { "readSize", bytebuf_readSize },
    { "readString", bytebuf_readString },
    { "clear", bytebuf_clear},
    { "appendBuffer", bytebuf_appendBuffer},
    { "loadFromFile", bytebuf_loadFromFile},
    { "__gc", bytebuf_delete },
    { 0, 0 }
};
static const luaL_Reg methods[] = {
    { "new", bytebuf_new },
    { "readBool", bytebuf_readBool },
    { "readByte", bytebuf_readByte },
    { "readShort", bytebuf_readShort },
    { "readFshort", bytebuf_readFshort },
    { "readInt", bytebuf_readInt },
    { "readFint", bytebuf_readFint },
    { "readLong", bytebuf_readLong },
    { "readFlong", bytebuf_readFlong },
    { "readFloat", bytebuf_readFloat },
    { "readDouble", bytebuf_readDouble },
    { "readSize", bytebuf_readSize },
    { "readString", bytebuf_readString },
    { "clear", bytebuf_clear},
    { "appendBuffer", bytebuf_appendBuffer},
    { "loadFromFile", bytebuf_loadFromFile},
    { 0, 0 }
};

LUALIB_API int luaopen_bytebuf(lua_State *L) {
    luaL_newmetatable(L, BYTE_BUF_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta_methods, 0);
    lua_pop(L, 1);
    luaL_newlib(L, methods);
    return 1;
}
