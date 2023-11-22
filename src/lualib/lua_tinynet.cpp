// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <cstdlib>
#include <thread>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <limits>
#include "logging/logging.h"
#include "lua_tinynet.h"
#include "lua_helper.h"
#include "lualib/lua_script.h"
#include "lua_compat.h"
#include "base/clock.h"
#include "lua_http.h"
#include "lua_timer.h"
#include "lua_redis.h"
#include "lua_mysql.h"
#include "lua_websocket.h"
#include "lua_cluster.h"
#include "lua_process.h"
#include "lua_allocator.h"
#include "lua_logger.h"
#include "lua_pb.h"
#include "lua_pugixml.h"
#include "base/crypto.h"
#include "lua_identifier.h"
#include "util/net_utils.h"
#include "app/app_container.h"
#include "base/application.h"
#include "util/fs_utils.h"
#include "util/sys_utils.h"
#include "util/process_utils.h"
#include "lua_proto_types.h"
#include "lua_socket.h"
#include "lua_yaml.h"
#include "lua_fs_types.h"
#include "lua_zlib.h"
#include "lua_fs.h"
#include "lua_base64.h"
#include "lua_rapidjson.h"
#include "lua_bytes.h"
#include "lua_aoi.h"
#include "lua_geo.h"
#include "lua_mmdb.h"
#include "lua_bytebuf.h"
#include "lua_textfilter.h"
#include "lua_tilemap.h"
#include "base/error_code.h"

using namespace tinynet;

static int lua_time(lua_State *L) {
    double seconds = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::system_clock::now().time_since_epoch()).count();
    lua_pushnumber(L, seconds);
    return 1;
}

static int lua_time_ms(lua_State *L) {
    lua_pushinteger(L, Time_ms());
    return 1;
}

static int lua_high_resolution_time(lua_State *L) {
    double seconds = std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    lua_pushnumber(L, seconds);
    return 1;
}


static int lua_sleep(lua_State *L) {
    double seconds = luaL_checknumber(L, 1);
    std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
    return 0;
}

static int lua_require_module(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    lua_settop(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    lua_getfield(L, 2, name);

    if (lua_toboolean(L, -1)) {
        return 1;
    }
    lua_pop(L, 2);
    lua_getglobal(L, "require");
    if (!lua_isfunction(L, -1)) {
        return luaL_error(L, "require function not found!");
    }
    lua_pushvalue(L, 1);
    if (luaL_pcall(L, 1, 1) != 0) {
        return 0;
    }
    return 1;
}

static int lua_new_uniqueid(lua_State *L) {
    auto app = lua_getapp(L);
    lua_pushidentifier(L, app->event_loop()->NewUniqueId());
    return 1;
}

static int lua_md5_sum(lua_State *L) {
    size_t dataLen;
    const char *data = luaL_checklstring(L, 1, &dataLen);
    std::string src(data, dataLen);
    std::string value = Crypto::bin2hex(Crypto::md5(src));
    lua_pushlstring(L, value.c_str(), value.length());
    return 1;
}

static int lua_sha1_sum(lua_State *L) {
    size_t dataLen;
    const char *data = luaL_checklstring(L, 1, &dataLen);
    std::string src(data, dataLen);
    std::string value = Crypto::bin2hex(Crypto::sha1(src));
    lua_pushlstring(L, value.c_str(), value.length());
    return 1;
}

static int lua_sha1(lua_State *L) {
    size_t dataLen;
    const char *data = luaL_checklstring(L, 1, &dataLen);
    std::string src(data, dataLen);
    std::string value = Crypto::sha1(src);
    lua_pushlstring(L, value.c_str(), value.length());
    return 1;
}

static int lua_sha256(lua_State *L) {
    size_t dataLen;
    const char *data = luaL_checklstring(L, 1, &dataLen);
    std::string src(data, dataLen);
    std::string value = Crypto::sha256(src);
    lua_pushlstring(L, value.c_str(), value.length());
    return 1;
}

static int lua_sha256_sum(lua_State *L) {
    size_t dataLen;
    const char *data = luaL_checklstring(L, 1, &dataLen);
    std::string src(data, dataLen);
    std::string value = Crypto::bin2hex(Crypto::sha256(src));
    lua_pushlstring(L, value.c_str(), value.length());
    return 1;
}

static int lua_crc16(lua_State *L) {
    size_t dataLen;
    const char *data = luaL_checklstring(L, 1, &dataLen);
    lua_pushinteger(L, Crypto::crc16(data, static_cast<int>(dataLen)));
    return 1;
}

static int lua_bin2hex(lua_State *L) {
    size_t dataLen;
    const char *data = luaL_checklstring(L, 1, &dataLen);
    std::string src(data, dataLen);
    std::string value = Crypto::bin2hex(src);
    lua_pushlstring(L, value.c_str(), value.length());
    return 1;
}

static int lua_hex2bin(lua_State *L) {
    size_t dataLen;
    const char *data = luaL_checklstring(L, 1, &dataLen);
    std::string src(data, dataLen);
    std::string value = Crypto::hex2bin(src);
    lua_pushlstring(L, value.c_str(), value.length());
    return 1;
}

static int lua_hash_hmac_alogs(lua_State *L) {
    std::vector<std::string> algo_vec;
    Crypto::hash_hmac_algos(algo_vec);
    LuaState S{ L };
    S << algo_vec;
    return 1;
}

static int lua_hash_hmac(lua_State *L) {
    size_t keyLen, dataLen;
    const char* algo = luaL_checkstring(L, 1);
    const char *data = luaL_checklstring(L, 2, &dataLen);
    const char *key = luaL_checklstring(L, 3, &keyLen);
    std::string in_alog(algo);
    std::string in_key(key, keyLen);
    std::string in_data(data, dataLen);
    std::string result = Crypto::hash_hmac(in_alog, in_key, in_data);
    lua_pushlstring(L, result.c_str(), result.length());
    return 1;
}

static int lua_openssl_sign(lua_State *L) {
    size_t keyLen, dataLen;
    const char *data = luaL_checklstring(L, 1, &dataLen);
    const char *key = luaL_checklstring(L, 2, &keyLen);
    const char* algo = luaL_checkstring(L, 3);

    std::string in_key(key, keyLen);
    std::string in_data(data, dataLen);
    std::string in_alog(algo);
    std::string result = Crypto::openssl_sign(in_data, in_key, in_alog);
    lua_pushlstring(L, result.c_str(), result.length());
    return 1;
}

static int lua_openssl_verify(lua_State *L) {
    size_t keyLen, dataLen, signLen;
    const char* data = luaL_checklstring(L, 1, &dataLen);
    const char* sign = luaL_checklstring(L, 2, &signLen);
    const char *key = luaL_checklstring(L, 3, &keyLen);
    const char* algo = luaL_checkstring(L, 4);

    std::string in_data(data, dataLen);
    std::string in_sign(sign, dataLen);
    std::string in_key(key, keyLen);
    std::string in_alog(algo);
    bool result = Crypto::openssl_verify(in_data, in_sign, in_key, in_alog);
    lua_pushboolean(L, result);
    return 1;
}


static int lua_id_to_string(lua_State *L) {
    int64_t guid = luaL_checkidentifier(L, 1);
    std::string str = std::to_string(guid);
    lua_pushstring(L, str.c_str());
    return 1;
}

static int lua_string_to_id(lua_State *L) {
    const char * str = luaL_checkstring(L, 1);
    int64_t guid = strtoll(str, NULL, 10);
    lua_pushidentifier(L, guid);
    return 1;
}

static int lua_iconv(lua_State *L) {
    size_t input_len = 0;
    const char* in_charset_str = luaL_checkstring(L, 1);
    const char* out_charset_str = luaL_checkstring(L, 2);
    const char* input_str = luaL_checklstring(L, 3, &input_len);
    std::string in_charset(in_charset_str);
    std::string out_charset(out_charset_str);
    std::string input, output;
    input.assign(input_str, input_len);
    StringUtils::iconv(in_charset, out_charset, input, output);
    lua_pushlstring(L, output.data(), output.size());
    return 1;
}

static int lua_integer_to_string(lua_State *L) {
    lua_Integer value = luaL_checkinteger(L, 1);
    std::string str = std::to_string(value);
    lua_pushstring(L, str.c_str());
    return 1;
}

static int lua_string_to_integer(lua_State *L) {
    const char * str = luaL_checkstring(L, 1);
    int64_t value = strtoll(str, NULL, 0);
    lua_pushinteger(L, value);
    return 1;
}

static int lua_to_boolean(lua_State *L) {
    bool result = false;
    int type = lua_type(L, 1);
    switch (type) {
    case LUA_TNIL: {
        result = false;
        break;
    }
    case LUA_TBOOLEAN: {
        result = (bool)lua_toboolean(L, 1);
        break;
    }
    case LUA_TNUMBER: {
        result = (bool)(lua_tonumber(L, 1));
        break;
    }
    case LUA_TSTRING: {
        const char* str = luaL_checkstring(L, 1);
        result = StringUtils::to_boolean(str);
        break;
    }
    default:
        break;
    }
    lua_pushboolean(L, result);
    return 1;
}

static int lua_get_ip(lua_State *L) {
    std::string ip;
    NetUtils::GetLocalIP(&ip);
    lua_pushstring(L, ip.c_str());
    return 1;
}

static int lua_exit(lua_State *L) {
    g_App->Shutdown();
    return 0;
}

static int lua_set_process_title(lua_State *L) {
    const char* title = luaL_checkstring(L, 1);
    ProcessUtils::set_process_title(title);
    return 0;
}

static int lua_get_exe_path(lua_State *L) {
    std::string path =  FileSystemUtils::fullpath(g_App->GetArgs()[0]);
    lua_pushstring(L, path.c_str());
    return 1;
}

static int lua_get_cpu_count(lua_State *L) {
    lua_pushnumber(L, SysUtils::get_cpu_count());
    return 1;
}

static int lua_get_os_name(lua_State *L) {
    lua_pushstring(L, SysUtils::get_os_name());
    return 1;
}

static int lua_get_cwd(lua_State *L) {
    std::string path;
    FileSystemUtils::get_cwd(&path);
    lua_pushlstring(L, path.data(), path.length());
    return 1;
}

static int lua_utf8_len(lua_State *L) {
    const char* str = luaL_checkstring(L, 1);
    size_t len = StringUtils::utf8_len(str);
    lua_pushnumber(L, static_cast<lua_Number>(len));
    return 1;
}

static int lua_openssl_encrypt(lua_State *L) {
    size_t dataLen, keyLen;
    std::string in_data, in_key, in_iv, in_tag, in_aad;
    std::string *iv_ptr, *aad_ptr;
    const char* data = luaL_checklstring(L, 1, &dataLen);
    const char* method = luaL_checkstring(L, 2);
    const char* key = luaL_checklstring(L, 3, &keyLen);
    in_data.assign(data, dataLen);
    in_key.assign(key, keyLen);
    iv_ptr = nullptr;
    aad_ptr = nullptr;
    int top = lua_gettop(L);
    if (top >= 4) {
        size_t ivLen;
        const char* iv = luaL_checklstring(L, 4, &ivLen);
        in_iv.assign(iv, ivLen);
        iv_ptr = &in_iv;
    }
    if (top >= 5) {
        size_t aadLen;
        const char* aad = luaL_checklstring(L, 6, &aadLen);
        in_aad.assign(aad, aadLen);
    }
    std::string tag;
    std::string result =  Crypto::openssl_encrypt(in_data, method, in_key, iv_ptr,aad_ptr, &tag);
    lua_pushlstring(L, result.data(), result.length());
    lua_pushlstring(L, tag.data(), tag.length());
    return 2;
}


static int lua_openssl_decrypt(lua_State *L) {
    size_t dataLen, keyLen;
    std::string in_data, in_key, in_iv, in_tag, in_aad;
    std::string *iv_ptr, *tag_ptr, *aad_ptr;
    const char* data = luaL_checklstring(L, 1, &dataLen);
    const char* method = luaL_checkstring(L, 2);
    const char* key = luaL_checklstring(L, 3, &keyLen);
    in_data.assign(data, dataLen);
    in_key.assign(key, keyLen);
    iv_ptr = nullptr;
    tag_ptr = nullptr;
    aad_ptr = nullptr;
    int top = lua_gettop(L);
    if (top >= 4) {
        size_t ivLen;
        const char* iv = luaL_checklstring(L, 4, &ivLen);
        in_iv.assign(iv, ivLen);
        iv_ptr = &in_iv;
    }
    if (top >= 5) {
        size_t aadLen;
        const char* aad = luaL_optlstring(L, 5, NULL, &aadLen);
        if (aad) {
            in_aad.assign(aad, aadLen);
            aad_ptr = &in_aad;
        }
    }
    if (top >= 6) {
        size_t tagLen;
        const char* tag = luaL_optlstring(L, 6, NULL, &tagLen);
        if (tag) {
            in_tag.assign(tag, tagLen);
            tag_ptr = &in_tag;
        }
    }
    std::string result = Crypto::openssl_decrypt(in_data, method, in_key, iv_ptr, aad_ptr, tag_ptr);
    lua_pushlstring(L, result.data(), result.length());
    return 1;
}

static int lua_openssl_cipher_iv_length(lua_State *L) {
    const char* name = luaL_checkstring(L, 1);
    Crypto::openssl_cipher_iv_length(name);
    lua_pushnumber(L, Crypto::openssl_cipher_iv_length(name));
    return 1;
}

static int lua_openssl_random_pseudo_bytes(lua_State *L) {
    lua_Number num = luaL_checknumber(L, 1);
    std::string result = Crypto::openssl_random_pseudo_bytes((int)num);
    lua_pushlstring(L, result.data(), result.length());
    return 1;
}

static void next_tick_callback(lua_State* L, int nref) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, nref);
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    if (!lua_isfunction(L, -1)) {
        log_warning("Next tick not found callback function!");
        lua_pop(L, 1);
        return;
    }
    luaL_pcall(L, 0, 0);
}

static int lua_next_tick(lua_State *L) {
    auto app = lua_getapp(L);
    auto LL = app->get<lua_State>();
    luaL_argcheck(L, lua_type(L, 1) == LUA_TFUNCTION, 1, "function expected!");
    lua_pushvalue(L, 1);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);

    auto taskId = app->event_loop()->AddTask(std::bind(next_tick_callback, LL, nref));
    lua_pushidentifier(L, taskId);
    return 1;
}

static int lua_tinynet_strerror(lua_State *L) {
    int code = (int)luaL_checkinteger(L, 1);
    auto err = tinynet_strerror(code);
    lua_pushstring(L, err);
    return 1;
}

static int luaopen_errorcode(lua_State *L) {
    lua_newtable(L);
    auto desc = tinynet::ErrorCode_descriptor();
    for (int i = 0; i < desc->value_count(); ++i) {
        auto value = desc->value(i);
        lua_pushinteger(L, value->number());
        lua_setfield(L, -2, value->name().c_str());
    }
    return 1;
}

static const luaL_Reg global_funcs[] = {
    {"time", lua_time },
    {"time_ms", lua_time_ms },
    {"high_resolution_time", lua_high_resolution_time},
    {"sleep", lua_sleep},
    {"require_module", lua_require_module},
    {"new_uniqueid", lua_new_uniqueid},
    {"md5_sum", lua_md5_sum},
    {"sha1_sum", lua_sha1_sum},
    {"sha1", lua_sha1},
    {"sha256_sum", lua_sha256_sum},
    {"sha256", lua_sha256},
    {"crc16", lua_crc16},
    {"hash_hmac_algos", lua_hash_hmac_alogs},
    {"hash_hmac", lua_hash_hmac},
    {"openssl_sign", lua_openssl_sign},
    {"openssl_verify", lua_openssl_verify},
    {"integer_to_string", lua_integer_to_string},
    {"string_to_integer", lua_string_to_integer},
    {"id_to_string", lua_id_to_string},
    {"string_to_id", lua_string_to_id},
    {"to_boolean", lua_to_boolean},
    {"iconv", lua_iconv},
    {"get_ip", lua_get_ip},
    {"exit", lua_exit},
    {"set_process_title", lua_set_process_title},
    {"get_exe_path", lua_get_exe_path},
    {"get_os_name", lua_get_os_name},
    {"get_cpu_count", lua_get_cpu_count},
    {"get_cwd", lua_get_cwd},
    {"utf8_len", lua_utf8_len},
    {"bin2hex", lua_bin2hex},
    {"hex2bin", lua_hex2bin},
    {"openssl_cipher_iv_length", lua_openssl_cipher_iv_length},
    {"openssl_random_pseudo_bytes", lua_openssl_random_pseudo_bytes},
    {"openssl_encrypt", lua_openssl_encrypt},
    {"openssl_decrypt", lua_openssl_decrypt},
    {"next_tick", lua_next_tick},
    {"tinynet_strerror", lua_tinynet_strerror},
    {0, 0}
};

static const luaL_Reg global_libs[] = {
    { "log", luaopen_logger },
    { "pb", luaopen_pb},
    { "allocator", luaopen_allocator},
    { "yaml", luaopen_yaml},
    { "zlib", luaopen_zlib},
    { "fs", luaopen_fs},
    { "base64", luaopen_base64},
    { "rjson", luaopen_rapidjson},
    { "bytes", luaopen_bytes},
    { "aoi", luaopen_aoi},
    { "geo", luaopen_geo},
    { "mmdb", luaopen_mmdb},
    { "bytebuf", luaopen_bytebuf},
    { "textfilter", luaopen_textfilter},
    { "tilemap", luaopen_tilemap},
    { 0, 0 }
};

static const luaL_Reg tinynet_members[] {
    {"timer", luaopen_timer},
    {"http", luaopen_http},
    {"redis", luaopen_redis},
    {"mysql", luaopen_mysql},
    {"ws", luaopen_ws},
    {"cluster", luaopen_cluster},
    {"process", luaopen_process},
    {"pugixml", luaopen_pugixml},
    {"socket", luaopen_socket},
    {"ErrorCode", luaopen_errorcode},
    {0, 0}
};
extern "C"
{
    int luaopen_cjson(lua_State *l);
    int luaopen_cjson_safe(lua_State *l);
    int luaopen_socket_core(lua_State *L);
}
static const luaL_Reg third_party_libs[] {
    { "cjson", luaopen_cjson },
    { "cjson.safe", luaopen_cjson_safe },
    { "socket.core", luaopen_socket_core },
    { 0, 0 }
};

LUALIB_API int luaopen_tinynet(lua_State *L) {
    //register global functions
    lua_pushglobaltable(L);
    luaL_setfuncs(L, global_funcs, 0);
    lua_pop(L, 1);

    const luaL_Reg *lib;
    //register global libs
    for (lib = global_libs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);
    }
    //register third party libs
    for (lib = third_party_libs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);
    }
    //register tinynet libs
    lua_newtable(L);
    int ret;
    for (lib = tinynet_members; lib->func; lib++) {
        ret = lib->func(L);
        if (ret == 1) {
            lua_setfield(L, -2, lib->name);
        } else {
            lua_pop(L, ret);
        }
    }
    return 1;
}

LUALIB_API void luaL_opentinynet(lua_State *L) {
    luaL_requiref(L, "tinynet", luaopen_tinynet, 1);
    lua_pop(L, 1);
}
