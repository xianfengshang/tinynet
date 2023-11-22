// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifndef __LUATYPES_H
#define __LUATYPES_H
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <limits>
#include "lua_helper.h"
#include "base/string_view.h"
#include "util/string_utils.h"
#include "base/io_buffer.h"
#include "lua_bytes.h"

struct LuaState {
    lua_State *L;
};

#define  LUA_READ_FIELD(NAME)\
	lua_pushstring(L.L, #NAME);\
	lua_rawget(L.L, -2);\
	L >> o.NAME; \
	lua_pop(L.L, 1)

#define  LUA_READ_FIELD_COND(NAME, COND)\
	if ((COND)) {\
		lua_pushstring(L.L, #NAME);\
		lua_rawget(L.L, -2);\
		L >> o.NAME; \
		lua_pop(L.L, 1);\
	}

#define  LUA_READ_FIELD_EX(NAME, DefaultValue)\
	lua_pushstring(L.L, #NAME);\
	lua_rawget(L.L, -2);\
	if (lua_isnil(L.L, -1)) {\
		o.NAME = DefaultValue;\
	}\
	else {\
		L >> o.NAME; \
	}\
	lua_pop(L.L, 1)

#define LUA_READ(TYPE)\
	inline const LuaState& operator >> (const LuaState& L, TYPE & o)

#define LUA_READ_BEGIN()\
	if (!lua_istable(L.L, -1))\
	{\
		return L;\
	}\
	int top = lua_gettop(L.L)

#define LUA_READ_END()\
	lua_settop(L.L, top);\
	return L

#define LUA_WRITE(TYPE)\
	inline LuaState& operator << (LuaState& L, const TYPE & o)

#define LUA_WRITE_BEGIN()\
	lua_newtable(L.L);\
	int top = lua_gettop(L.L);\
	int array_idx = 0;\
	(void)array_idx

#define  LUA_WRITE_END()\
	lua_settop(L.L, top);\
	return L

#define LUA_WRITE_FIELD(NAME)\
	lua_pushstring(L.L, #NAME);\
	L << o.NAME;\
	lua_rawset(L.L, -3)

#define LUA_WRITE_FIELD_EX(NAME, VALUE)\
	lua_pushstring(L.L, #NAME);\
	L << VALUE;\
	lua_rawset(L.L, -3)

#define LUA_WRITE_FIELD_COND(NAME, COND)\
	if ((COND)) {\
		lua_pushstring(L.L, #NAME);\
		L << o.NAME;\
		lua_rawset(L.L, -3);\
	}

#define LUA_WRITE_FIELD_COND_EX(NAME, VALUE, COND)\
	if ((COND)) {\
		lua_pushstring(L.L, #NAME);\
		L << VALUE;\
		lua_rawset(L.L, -3);\
	}

#define LUA_WRITE_ARRAY_FIELD(NAME)\
	L << o.NAME;\
	lua_rawseti(L.L, -2, ++array_idx)

#define LUA_WRITE_ARRAY_FIELD_COND(NAME, COND)\
	if ((COND)) {\
		L << o.NAME;\
		lua_rawseti(L.L, -2, ++array_idx);\
	}

#define  LUA_WRITE_CONST(NAME) \
	lua_pushstring(L, #NAME);\
	lua_pushnumber(L, NAME);\
	lua_rawset(L, -3)

#define LUA_WRITE_ENUM(NAME) \
	lua_pushstring(L, #NAME);\
	lua_pushinteger(L, NAME);\
	lua_rawset(L, -3)


inline LuaState& operator << (LuaState& L, const char* o) {
    lua_pushstring(L.L, o);
    return L;
}


inline const LuaState& operator >> (const LuaState& L, std::string& o) {
    //size_t len = 0;
    //if (lua_isstring(L.L, -1)) {
    //    const char* value = lua_tolstring(L.L, -1, &len);
    //    o.assign(value, len);
    //}
    auto ret = luaL_tobytes(L.L, -1, &o);
    if (ret != &o)
        o.assign(*ret);
    return L;
}

inline LuaState& operator << (LuaState& L, const std::string& o) {
    lua_pushlstring(L.L, o.c_str(), o.length());
    return L;
}

inline LuaState& operator << (LuaState& L, const std::string* o) {
    lua_pushbytes(L.L, o);
    return L;
}

inline const LuaState& operator >> (const LuaState& L, tinynet::string_view& o) {
    if (lua_isstring(L.L, -1)) {
        size_t len = 0;
        const char* value = lua_tolstring(L.L, -1, &len);
        o = tinynet::string_view(value, len);
    }
    return L;
}

inline LuaState& operator << (LuaState& L, const tinynet::string_view& o) {
    lua_pushlstring(L.L, o.data(), o.size());
    return L;
}

inline const LuaState& operator >> (const LuaState& L, tinynet::iov_t& o) {
    if (lua_isstring(L.L, -1)) {
        size_t len = 0;
        o.base = const_cast<char*>(lua_tolstring(L.L, -1, &len));
        o.len = static_cast<unsigned long>(len);
    }
    return L;
}

inline const LuaState& operator >> (const LuaState& L, bool& o) {
    if (lua_isboolean(L.L, -1)) {
        o = lua_toboolean(L.L, -1);
    } else {
        if (lua_isstring(L.L, -1)) {
            const char* value = lua_tostring(L.L, -1);
            o = StringUtils::to_boolean(value);
        }
    }
    return L;
}

inline LuaState& operator << (LuaState& L, bool o) {
    lua_pushboolean(L.L, o);
    return L;
}

inline const LuaState& operator >> (const LuaState& L, int& o) {
    if (lua_isnumber(L.L, -1))
        o = static_cast<int>(lua_tointeger(L.L, -1));
    return L;
}

inline LuaState& operator << (LuaState& L, int o) {
    lua_pushinteger(L.L, o);
    return L;
}

inline const LuaState& operator >> (const LuaState& L, unsigned int& o) {
    if (lua_isnumber(L.L, -1))
        o = static_cast<int>(lua_tointeger(L.L, -1));
    return L;
}

inline LuaState& operator << (LuaState& L, unsigned int o) {
    lua_pushinteger(L.L, o);
    return L;
}

#if defined(__LP64__)
#else
inline const LuaState& operator >> (const LuaState& L, long &o) {
    if (lua_isnumber(L.L, -1))
        o = static_cast<long>(lua_tointeger(L.L, -1));
    return L;
}

inline LuaState& operator << (LuaState& L, long o) {
    lua_pushinteger(L.L, o);
    return L;
}

inline const LuaState& operator >> (const LuaState& L, unsigned long &o) {
    if (lua_isnumber(L.L, -1))
        o = static_cast<unsigned long>(lua_tointeger(L.L, -1));
    return L;
}

inline LuaState& operator << (LuaState& L, unsigned long o) {
    lua_pushinteger(L.L, o);
    return L;
}
#endif

inline const LuaState& operator >> (const LuaState& L, int64_t &o) {
    int type = lua_type(L.L, -1);
    if (type == LUA_TNUMBER) {
        o = lua_tointeger(L.L, -1);
    } else if (type == LUA_TSTRING) {
        o = std::atoll(lua_tostring(L.L, -1));
    }
    return L;
}

inline LuaState& operator << (LuaState& L, int64_t o) {
    lua_pushinteger(L.L, o);
    return L;
}

inline const LuaState& operator >> (const LuaState& L, uint64_t &o) {
    int type = lua_type(L.L, -1);
    if (type == LUA_TNUMBER) {
        o = lua_tointeger(L.L, -1);
    } else if (type == LUA_TSTRING) {
        o = std::strtoull(lua_tostring(L.L, -1), NULL, 10);
    }
    return L;
}

inline LuaState& operator << (LuaState& L, uint64_t o) {
#if (LUA_VERSION_NUM >= 503)
    lua_pushinteger(L.L, static_cast<lua_Integer>(o));
#else
    lua_pushnumber(L.L, static_cast<lua_Number>(o));
#endif
    return L;
}

inline const LuaState& operator >> (const LuaState& L, double& o) {
    if (lua_isnumber(L.L, -1))
        o = static_cast<double>(lua_tonumber(L.L, -1));
    return L;
}

inline LuaState& operator << (LuaState& L, double o) {
    lua_pushnumber(L.L, o);
    return L;
}

inline const LuaState& operator >> (const LuaState& L, float& o) {
    if (lua_isnumber(L.L, -1))
        o = static_cast<float>(lua_tonumber(L.L, -1));
    return L;
}

inline LuaState& operator << (LuaState& L, float o) {
    lua_pushnumber(L.L, static_cast<double>(o));
    return L;
}


inline const LuaState& operator >> (const LuaState& L, void * &o) {
    if (lua_islightuserdata(L.L, -1))
        o = lua_touserdata(L.L, -1);
    return L;
}

inline LuaState& operator << (LuaState& L, void *o) {
    if (o == nullptr) {
        lua_pushnil(L.L);
    } else {
        lua_pushlightuserdata(L.L, o);
    }
    return L;
}


inline const LuaState& operator >> (const LuaState& L, std::tuple<int, int> &o) {
    if (lua_istable(L.L, -1)) {
        int n = static_cast<int>(luaL_len(L.L, -1));
        if (n >= 2) {
            lua_rawgeti(L.L, -1, 1);
            L >> std::get<0>(o);
            lua_pop(L.L, 1);

            lua_rawgeti(L.L, -1, 2);
            L >> std::get<1>(o);
            lua_pop(L.L, 1);

        }
    }
    return L;
}

inline const LuaState& operator >> (const LuaState& L, std::tuple<float, float> &o) {
    if (lua_istable(L.L, -1)) {
        int n = static_cast<int>(luaL_len(L.L, -1));
        if (n >= 2) {
            lua_rawgeti(L.L, -1, 1);
            L >> std::get<0>(o);
            lua_pop(L.L, 1);

            lua_rawgeti(L.L, -1, 2);
            L >> std::get<1>(o);
            lua_pop(L.L, 1);

        }
    }
    return L;
}

inline const LuaState& operator >> (const LuaState& L, std::tuple<double, double> &o) {
    if (lua_istable(L.L, -1)) {
        int n = static_cast<int>(luaL_len(L.L, -1));
        if (n >= 2) {
            lua_rawgeti(L.L, -1, 1);
            L >> std::get<0>(o);
            lua_pop(L.L, 1);

            lua_rawgeti(L.L, -1, 2);
            L >> std::get<1>(o);
            lua_pop(L.L, 1);

        }
    }
    return L;
}

inline LuaState& operator << (LuaState& L, const std::pair<std::string, std::string> &o) {
    lua_newtable(L.L);
    LUA_WRITE_FIELD(first);
    LUA_WRITE_FIELD(second);
    return L;
}

inline const LuaState& operator >> (const LuaState& L, std::pair<std::string, std::string> &o) {
    if (lua_istable(L.L, -1)) {
        LUA_READ_FIELD(first);
        LUA_READ_FIELD(second);
    }
    return L;
}

inline LuaState& operator << (LuaState& L, std::tuple<int, int> &o) {
    lua_newtable(L.L);
    L << std::get<0>(o);
    lua_rawseti(L.L, -2, 1);
    L << std::get<1>(o);
    lua_rawseti(L.L, -2, 2);
    return L;
}

inline LuaState& operator << (LuaState& L, const std::tuple<float, float> &o) {
    lua_newtable(L.L);
    L << std::get<0>(o);
    lua_rawseti(L.L, -2, 1);
    L << std::get<1>(o);
    lua_rawseti(L.L, -2, 2);
    return L;
}

inline LuaState& operator << (LuaState& L, const std::tuple<double, double> &o) {
    lua_newtable(L.L);
    L << std::get<0>(o);
    lua_rawseti(L.L, -2, 1);
    L << std::get<1>(o);
    lua_rawseti(L.L, -2, 2);
    return L;
}


template<typename T>
inline const LuaState& operator >> (const LuaState& L, std::vector<T>& o) {
    if (lua_istable(L.L, -1)) {
        int n = static_cast<int>(luaL_len(L.L, -1));
        o.resize(n);
        for (int i = 0; i < n; ++i) {
            lua_rawgeti(L.L, -1, (i+1));
            L >> o[i];
            lua_pop(L.L, 1);
        }
    }
    return L;
}

template<typename T>
inline LuaState& operator << (LuaState& L, const std::vector<T>& o) {
    lua_newtable(L.L);
    for (size_t i = 0; i < o.size(); ++i) {
        L << o[i];
        lua_rawseti(L.L, -2, static_cast<int>(i + 1));
    }
    return L;
}

template<>
inline LuaState& operator << (LuaState& L, const tinynet::iovs_t& o) {
    luaL_Buffer b;
    luaL_buffinit(L.L, &b);
    for (size_t i = 0; i < o.size(); ++i) {
        luaL_addlstring(&b, o[i].base, o[i].len);
    }
    luaL_pushresult(&b);
    return L;
}

template<typename K, typename V>
inline const LuaState& operator >> (const LuaState& L, std::map<K, V>& o) {
    if (lua_istable(L.L, -1)) {
        lua_pushnil(L.L);
        while (lua_next(L.L, -2)) {
            K k;
            V v;
            L >> v;
            lua_pushvalue(L.L, -2);
            L >> k;
            lua_pop(L.L, 2);
            o.emplace(k, v);
        }
    }
    return L;
}

template<typename K, typename V>
inline LuaState& operator << (LuaState& L, const std::map<K, V>& o) {
    lua_newtable(L.L);
    for (auto &item : o) {
        L << item.first;
        L << item.second;
        lua_rawset(L.L, -3);
    }
    return L;
}

#endif // !__LUATYPES_H
