// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <memory>
#include <functional>
#include <algorithm>
#include "lua_mmdb.h"
#include "lua_helper.h"
#include "lua_script.h"
#include "lua_compat.h"
#include "lua_proto_types.h"
#include "maxminddb.h"
#include "util/string_utils.h"

#define  MMDB_META_TABLE "mmdb_meta_table"

struct MMDB : public MMDB_s {
    bool closed;
};

struct EntryDataList {
    typedef MMDB_entry_data_list_s* iterator;
    MMDB_entry_data_list_s* begin;
    MMDB_entry_data_list_s* end;
    EntryDataList():
        begin(nullptr),
        end(nullptr) {
    }
    EntryDataList(MMDB_entry_data_list_s* list) {
        begin = list;
        end = nullptr;
    }
    ~EntryDataList() {
        if (begin) {
            MMDB_free_entry_data_list(begin);
            begin = nullptr;
        }
    }
};

class EntryEventHandler {
  public:
    virtual void StartArray() = 0;
    virtual void EndArray() = 0;
    virtual void StartMap() = 0;
    virtual void EndMap() = 0;
    virtual void MapEntry() = 0;
    virtual void ArrayEntry(int index) = 0;
};

class ContainerParser {
  public:
    ContainerParser(EntryEventHandler* handler, int type, size_t size):
        handler_(handler),
        type_(type),
        size_(size),
        index_(0),
        inited_(false) {
    }
    ContainerParser(const ContainerParser& o) = default;
    ContainerParser(ContainerParser&& o) = default;
  public:
    void Parse() {
        if (type_ == MMDB_DATA_TYPE_MAP) {
            if (inited_)
                ParseMap();
            else
                InitMap();
        }
        if (type_ == MMDB_DATA_TYPE_ARRAY) {
            if (inited_)
                ParseArray();
            else
                InitArray();
        }
    }
  private:
    void InitMap() {
        inited_ = true;
        if (handler_)handler_->StartMap();
        if (size_ == 0) {
            if (handler_) handler_->EndMap();
        }
    }
    void InitArray() {
        inited_ = true;
        handler_->StartArray();
        if (size_ == 0) {
            handler_->EndArray();
        }
    }
    void ParseMap() {
        ++index_;
        if (index_ % 2 == 0) {
            if (handler_)handler_->MapEntry();
        }
        if (static_cast<size_t>(index_) >= 2 * size_) {
            if (handler_)handler_->EndMap();
        }
    }
    void ParseArray() {
        ++index_;
        if (handler_)handler_->ArrayEntry(index_);

        if (static_cast<size_t>(index_) >= size_) {
            if (handler_) handler_->EndArray();
        }
    }
  protected:
    EntryEventHandler* handler_;
    int type_;
    size_t size_;
    int index_;
    bool inited_;
};



class EntryDataListParser: EntryEventHandler {
  public:
    EntryDataListParser(lua_State* L):
        L_(L) {}
  public:
    void StartArray() {
        lua_newtable(L_);
    }
    void EndArray() {
        stack_.pop_back();
        commit();
    }
    void StartMap() {
        lua_newtable(L_);
    }
    void EndMap() {
        stack_.pop_back();
        commit();
    }
    void MapEntry() {
        lua_rawset(L_, -3);
    }
    void ArrayEntry(int index) {
        lua_rawseti(L_, -2, index);
    }
  public:
    void Encode(const EntryDataList& o) {
        for (EntryDataList::iterator it = o.begin; it != o.end; it = it->next) {
            if (!it->entry_data.has_data) {
                luaL_error(L_, "Invalid entry");
                return;
            }
            switch (it->entry_data.type) {
            case MMDB_DATA_TYPE_UTF8_STRING: {
                lua_pushlstring(L_, it->entry_data.utf8_string, it->entry_data.data_size);
                break;
            }
            case MMDB_DATA_TYPE_MAP: {
                stack_.push_back(ContainerParser(this, MMDB_DATA_TYPE_MAP, it->entry_data.data_size));
                break;
            }
            case MMDB_DATA_TYPE_ARRAY: {
                stack_.push_back(ContainerParser(this, MMDB_DATA_TYPE_ARRAY, it->entry_data.data_size));
                break;
            }
            case MMDB_DATA_TYPE_DOUBLE: {
                lua_pushnumber(L_, it->entry_data.double_value);
                break;
            }
            case MMDB_DATA_TYPE_UINT16: {
                lua_pushinteger(L_, it->entry_data.uint16);
                break;
            }
            case MMDB_DATA_TYPE_UINT32: {
                lua_pushinteger(L_, static_cast<lua_Integer>(it->entry_data.uint32));
                break;
            }
            case MMDB_DATA_TYPE_INT32: {
                lua_pushinteger(L_, static_cast<lua_Integer>(it->entry_data.uint32));
                break;
            }
            case MMDB_DATA_TYPE_UINT64: {
                lua_pushnumber(L_, static_cast<lua_Number>(it->entry_data.uint64));
                break;
            }
            case MMDB_DATA_TYPE_FLOAT: {
                lua_pushnumber(L_, it->entry_data.float_value);
                break;
            }
            case MMDB_DATA_TYPE_BOOLEAN: {
                lua_pushboolean(L_, it->entry_data.boolean);
                break;
            }
            case MMDB_DATA_TYPE_EXTENDED:
            case MMDB_DATA_TYPE_POINTER:
            case MMDB_DATA_TYPE_CONTAINER:
            case MMDB_DATA_TYPE_END_MARKER: {
                luaL_error(L_, "Data corrupt");
                return;
            }
            case MMDB_DATA_TYPE_BYTES: {
                lua_pushlstring(L_, (const char*)it->entry_data.bytes, it->entry_data.data_size);
                break;;
            }
            case MMDB_DATA_TYPE_UINT128: {
                lua_pushnil(L_); //Not supported
                break;
            }
            default: {
                luaL_error(L_, "Data type %d not supported", it->entry_data.type);
                return;
            }
            }
            commit();

        }
    }
  private:
    void commit() {
        if (stack_.empty()) return;
        stack_.back().Parse();
    }
  private:
    std::vector<ContainerParser> stack_;
    lua_State* L_;
};


static MMDB* luaL_checkmmdb(lua_State *L, int idx) {
    auto ud = (MMDB**)luaL_checkudata(L, idx, MMDB_META_TABLE);
    return *ud;
}

static int mmdb_open(lua_State *L) {
    const char* filename = luaL_checkstring(L, 1);
    std::unique_ptr<MMDB> mmdb(new (std::nothrow) MMDB());
    if (!mmdb) {
        std::string err;
        StringUtils::Format(err, "Failed to open the mmdb database %s: out of memory", filename);
        lua_pushnil(L);
        lua_pushlstring(L, err.data(), err.length());
        return 2;
    }
    const int status = MMDB_open(filename, MMDB_MODE_MMAP, mmdb.get());
    if (status != MMDB_SUCCESS) {
        std::string err;
        StringUtils::Format(err, "Failed to open the mmdb database %s: %s", filename, MMDB_strerror(status));
        lua_pushnil(L);
        lua_pushlstring(L, err.data(), err.length());
        return 2;
    }
    mmdb->closed = false;
    auto ud = (MMDB**)lua_newuserdata(L, sizeof(MMDB*));
    *ud = mmdb.release();
    luaL_getmetatable(L, MMDB_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int mmdb_close(lua_State *L) {
    auto mmdb = luaL_checkmmdb(L, 1);
    if (!mmdb->closed) {
        MMDB_close(mmdb);
        mmdb->closed = true;
    }
    return 0;
}

static int mmdb_delete(lua_State *L) {
    auto ud = (MMDB**)luaL_checkudata(L, 1, MMDB_META_TABLE);
    if (*ud == NULL) {
        return 0;
    }
    auto mmdb = *ud;
    if (!mmdb->closed) {
        MMDB_close(mmdb);
        mmdb->closed = true;
    }
    std::unique_ptr<MMDB> ptr(mmdb);
    (void)ptr;
    *ud = NULL;
    return 0;
}

static int mmdb_version(lua_State *L) {
    lua_pushstring(L, MMDB_lib_version());
    return 1;
}

static int mmdb_lookup(lua_State *L) {
    auto mmdb = luaL_checkmmdb(L, 1);
    const char* ip = luaL_checkstring(L, 2);
    int gai_err = 0;
    int mmdb_err = 0;
    MMDB_lookup_result_s result = MMDB_lookup_string(mmdb, ip, &gai_err, &mmdb_err);
    if (gai_err) {
        return luaL_error(L, gai_strerror(gai_err));
    }
    if (mmdb_err) {
        return luaL_error(L, MMDB_strerror(mmdb_err));
    }
    if (!result.found_entry) {
        return luaL_error(L, "Failed to find address:%s", ip);
    }
    EntryDataList list;
    const int status = MMDB_get_entry_data_list(&result.entry, &list.begin);
    if (status) {
        return luaL_error(L, MMDB_strerror(status));
    }
    int top = lua_gettop(L);
    EntryDataListParser parser(L);
    parser.Encode(list);
    int nret = lua_gettop(L) - top;
    return nret;
}

static const luaL_Reg meta_methods[] = {
    { "lookup", mmdb_lookup},
    { "close", mmdb_close},
    { "__gc", mmdb_delete},
    {0, 0}
};
static const luaL_Reg methods[] = {
    {"open", mmdb_open },
    {"version", mmdb_version },
    { 0, 0 }
};

LUALIB_API int luaopen_mmdb(lua_State *L) {
    luaL_newmetatable(L, MMDB_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta_methods, 0);
    lua_pop(L, 1);
    luaL_newlib(L, methods);
    return 1;
}
