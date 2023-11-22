// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <memory>
#include <functional>
#include <unordered_map>
#include "lua_redis.h"
#include "redis/redis_channel.h"
#include "redis/redis_client.h"
#include "lua_helper.h"
#include "lua_script.h"
#include "lua_compat.h"
#include "lua_proto_types.h"
#include "base/string_view.h"

#define  REDIS_META_TABLE "redis_meta_table"

using namespace tinynet;

class LuaRedisClient {
  public:
    LuaRedisClient(app::AppContainer* app) :
        app_(app),
        monitor_callback_ref_(LUA_REFNIL) {
        client_.reset(new(std::nothrow) redis::RedisClient(app->event_loop()));
    }
    ~LuaRedisClient() {
        set_monitor_callback(LUA_REFNIL);
        clear_subscribe_callback();
    }
  public:
    void Init(const redis::RedisOptions& options) {
        client_->Init(options);
    }

    int Command(const tinynet::string_view& cmd, int nref) {
        return client_->Command(cmd, std::bind(&LuaRedisClient::OnCommand, this, nref, std::placeholders::_1));
    }

    int CommandArgv(const std::vector<tinynet::string_view>& iovs, int nref) {
        return client_->CommandArgv(iovs, std::bind(&LuaRedisClient::OnCommand, this, nref, std::placeholders::_1));
    }

    int Monitor(int nref) {
        int err =  client_->Monitor(std::bind(&LuaRedisClient::OnMonitor, this, std::placeholders::_1));
        if (err == ERROR_OK) {
            set_monitor_callback(nref);
        }
        return err;
    }

    int Subscribe(const std::string& name, int nref) {
        auto channel = client_->CreateSubscribeChannel(name);
        if (!channel) {
            return ERROR_REDIS_SUBSCRIBE;
        }
        int err = client_->Subscribe(name, std::bind(&LuaRedisClient::OnSubscribe, this, channel->get_guid(), std::placeholders::_1));
        if (err == ERROR_OK) {
            set_subscribe_callback(channel->get_guid(), nref);
        }
        return err;
    }

    void Unsubscribe(const std::string& name) {
        auto channel = client_->GetSubscribeChannel(name);
        if (channel) {
            set_subscribe_callback(channel->get_guid(), LUA_REFNIL);
            client_->Unsubscribe(name);
        }
    }

    int Psubscribe(const std::string& pattern, int nref) {
        auto channel = client_->CreateSubscribeChannel(pattern);
        if (!channel) {
            return ERROR_REDIS_SUBSCRIBE;
        }
        int err = client_->Psubscribe(pattern, std::bind(&LuaRedisClient::OnSubscribe, this, channel->get_guid(), std::placeholders::_1));
        if (err == ERROR_OK) {
            set_subscribe_callback(channel->get_guid(), nref);
        }
        return err;
    }

    void Punsubscribe(const std::string& pattern) {
        auto channel = client_->GetSubscribeChannel(pattern);
        if (channel) {
            set_subscribe_callback(channel->get_guid(), LUA_REFNIL);
            client_->Punsubscribe(pattern);
        }
    }
  private:
    void OnCommand(int nref, const redis::RedisReply& reply) {
        lua_State *L = app_->get<lua_State>();
        lua_rawgeti(L, LUA_REGISTRYINDEX, nref);
        luaL_unref(L, LUA_REGISTRYINDEX, nref);
        if (!lua_isfunction(L, -1)) {
            log_warning("Redis reply can not found callback function!");
            lua_pop(L, 1);
            return;
        }
        int nargs;
        if (reply.type == redis::REDIS_REPLY_ERROR) {
            nargs = 2;
            lua_pushnil(L);
            lua_pushlstring(L, reply.str.data(), reply.str.length());
        } else {
            nargs = 1;
            LuaState S{ L };
            S << reply;
        }

        luaL_pcall(L, nargs, 0);
    }

    void OnMonitor(const redis::RedisReply& reply) {
        if (monitor_callback_ref_ == LUA_REFNIL) {
            return;
        }
        lua_State *L = app_->get<lua_State>();
        lua_rawgeti(L, LUA_REGISTRYINDEX, monitor_callback_ref_);
        if (!lua_isfunction(L, -1)) {
            log_warning("RedisClient monitor can not found callback function!");
            lua_pop(L, 1);
            return;
        }
        LuaState S = { L };
        S << reply;
        luaL_pcall(L, 1, 0);
    }

    void OnSubscribe(int64_t guid, const redis::RedisReply& reply) {
        auto it = subscribe_callback_refs_.find(guid);
        if (it == subscribe_callback_refs_.end()) {
            return;
        }
        lua_State *L = app_->get<lua_State>();
        lua_rawgeti(L, LUA_REGISTRYINDEX, it->second);
        if (!lua_isfunction(L, -1)) {
            log_warning("RedisClient subscribe can not found callback function!");
            lua_pop(L, 1);
            return;
        }
        LuaState S = { L };
        S << reply;
        luaL_pcall(L, 1, 0);
    }
  private:
    void set_monitor_callback(int nref) {
        if (monitor_callback_ref_ != LUA_REFNIL && monitor_callback_ref_ != nref) {
            lua_State *L = app_->get<lua_State>();
            luaL_unref(L, LUA_REGISTRYINDEX, monitor_callback_ref_);
        }
        monitor_callback_ref_ = nref;
    }
    void set_subscribe_callback(int64_t guid, int nref) {
        auto it = subscribe_callback_refs_.find(guid);
        if (it != subscribe_callback_refs_.end() && it->second != nref) {
            lua_State *L = app_->get<lua_State>();
            luaL_unref(L, LUA_REGISTRYINDEX, it->second);
            subscribe_callback_refs_.erase(it);
        }
        if (nref != LUA_REFNIL) {
            subscribe_callback_refs_[guid] = nref;
        }
    }
    void clear_subscribe_callback() {
        lua_State *L = app_->get<lua_State>();
        for (auto& it : subscribe_callback_refs_) {
            luaL_unref(L, LUA_REGISTRYINDEX, it.second);
        }
        subscribe_callback_refs_.clear();
    }
  private:
    app::AppContainer* app_;
    std::unique_ptr<redis::RedisClient> client_;
    int monitor_callback_ref_;
    std::unordered_map<int64_t, int> subscribe_callback_refs_;


};
static LuaRedisClient* luaL_checkredis(lua_State *L, int idx) {
    return (LuaRedisClient*)luaL_checkudata(L, idx, REDIS_META_TABLE);
}

static int redis_command(lua_State *L) {
    auto redis_client = luaL_checkredis(L, 1);
    int type = lua_type(L, 2);
    luaL_argcheck(L, type == LUA_TSTRING || type == LUA_TTABLE, 2, "string or string array expected");
    luaL_argcheck(L, lua_type(L, 3) == LUA_TFUNCTION, 3, "function expected");
    std::vector<tinynet::string_view> iovs;
    lua_pushvalue(L, 2);
    LuaState S{ L };
    if (type == LUA_TSTRING) {
        iovs.resize(1);
        S >> iovs[0];
    } else {
        S >> iovs;
    }
    lua_pop(L, 1);
    lua_pushvalue(L, 3);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);
    int err = redis_client->CommandArgv(iovs, nref);
    if (err == tinynet::ERROR_OK) {
        return 0;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    lua_pushstring(L, tinynet_strerror(err));
    return 1;
}

static int redis_monitor(lua_State *L) {
    auto redis_client = luaL_checkredis(L, 1);
    luaL_argcheck(L, lua_type(L, 2) == LUA_TFUNCTION, 2, "function expected");
    lua_pushvalue(L, 2);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);
    int err =  redis_client->Monitor(nref);
    if (err == tinynet::ERROR_OK) {
        return 0;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    lua_pushstring(L, tinynet_strerror(err));
    return 1;
}

static int redis_subscribe(lua_State *L) {
    auto redis_client = luaL_checkredis(L, 1);
    const char* name = luaL_checkstring(L, 2);
    luaL_argcheck(L, lua_type(L, 3) == LUA_TFUNCTION, 3, "function expected");
    lua_pushvalue(L, 3);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);
    std::string channel_name(name);
    int err = redis_client->Subscribe(channel_name, nref);
    if (err == tinynet::ERROR_OK) {
        return 0;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    lua_pushstring(L, tinynet_strerror(err));
    return 1;
}

static int redis_unsubscribe(lua_State *L) {
    auto redis_client = luaL_checkredis(L, 1);
    const char* name = luaL_checkstring(L, 2);
    std::string channel_name(name);
    redis_client->Unsubscribe(channel_name);
    return 0;
}

static int redis_psubscribe(lua_State *L) {
    auto redis_client = luaL_checkredis(L, 1);
    const char* pattern = luaL_checkstring(L, 2);
    luaL_argcheck(L, lua_type(L, 3) == LUA_TFUNCTION, 3, "function expected");
    lua_pushvalue(L, 3);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);
    std::string channel_name(pattern);
    int err = redis_client->Psubscribe(channel_name, nref);
    if (err == tinynet::ERROR_OK) {
        return 0;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    lua_pushstring(L, tinynet_strerror(err));
    return 1;
}

static int redis_punsubscribe(lua_State *L) {
    auto redis_client = luaL_checkredis(L, 1);
    const char* pattern = luaL_checkstring(L, 2);
    std::string channel_name(pattern);
    redis_client->Punsubscribe(channel_name);
    return 0;
}

static int redis_new(lua_State *L) {
    auto app = lua_getapp(L);
    LuaState S{ L };
    redis::RedisOptions options;
    S >> options;
    void* ud = lua_newuserdata(L, sizeof(LuaRedisClient));
    memset(ud, 0, sizeof(LuaRedisClient));
    auto redis_client =  new(ud) LuaRedisClient(app);
    redis_client->Init(options);
    luaL_getmetatable(L, REDIS_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int redis_delete(lua_State *L) {
    LuaRedisClient* redis_client = luaL_checkredis(L, 1);
    redis_client->~LuaRedisClient();
    return 0;
}

static const luaL_Reg meta_methods[] = {
    { "command", redis_command },
    { "monitor", redis_monitor },
    { "subscribe", redis_subscribe },
    { "unsubscribe", redis_unsubscribe },
    { "psubscribe", redis_psubscribe },
    { "punsubscribe", redis_punsubscribe },
    {"__gc", redis_delete },
    {0, 0}
};
static const luaL_Reg methods[] = {
    {"new", redis_new },
    { 0, 0 }
};

LUALIB_API int luaopen_redis(lua_State *L) {
    luaL_newmetatable(L, REDIS_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta_methods, 0);
    lua_pop(L, 1);
    luaL_newlib(L, methods);
    return 1;
}
