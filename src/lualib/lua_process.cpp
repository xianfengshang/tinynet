// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_process.h"
#include "lua_helper.h"
#include "logging/logging.h"
#include "lua_compat.h"
#include "app/app_container.h"
#include "lua_process_types.h"
#include "base/error_code.h"
#include "lua_types.h"
#include "lua_proto_types.h"
#include "process/process.h"

#define  PROCES_META_TABLE  "process_meta_table"

using namespace tinynet;

class LuaProcess: public  process::ProcessEventHandler {
  public:
    LuaProcess(app::AppContainer * app):
        app_(app),
        nref_(LUA_REFNIL) {
        process_ = app_->event_loop()->NewObject<process::Process>(this);
    }
    ~LuaProcess() {
        set_event_callback(LUA_REFNIL);
    }
  public:
    void HandleExit(int exit_status, int term_signal) override {
        lua::ProcessEvent evt;
        evt.type = "onexit";
        evt.exit_status = exit_status;
        evt.term_signal = term_signal;
        emit(evt);
    }
  public:
    int Spawn(const process::ProcessOptions& options) {
        int err = process_->Spawn(options);
        return err;
    }
    int Kill(int signum) {
        return process_->Kill(signum);
    }
    void Close() {
        process_->Close();
    }
    int GetPid() {
        return process_->GetPid();
    }
  public:
    void set_event_callback(int nref) {
        if (nref_ != LUA_REFNIL && nref_ != nref) {
            lua_State *L = app_->get<lua_State>();
            luaL_unref(L, LUA_REGISTRYINDEX, nref_);
        }
        nref_ = nref;
    }
  public:
    void emit(const tinynet::lua::ProcessEvent& event) {
        if (nref_ == LUA_REFNIL) {
            return;
        }
        lua_State *L = app_->get<lua_State>();

        lua_rawgeti(L, LUA_REGISTRYINDEX, nref_);
        if (!lua_isfunction(L, -1)) {
            log_warning("Process emit event can not found callback function!");
            lua_pop(L, 1);
            return;
        }
        LuaState S = { L };
        S << event;
        luaL_pcall(L, 1, 0);
    }
  private:
    app::AppContainer* app_;
    std::shared_ptr <process::Process> process_;
    int nref_;
};


static LuaProcess* luaL_checkprocess(lua_State *L, int idx) {
    return (LuaProcess*)luaL_checkudata(L, idx, PROCES_META_TABLE);
}

static int lua_process_new(lua_State *L) {
    auto app = lua_getapp(L);
    auto ptr = lua_newuserdata(L, sizeof(LuaProcess));
    new(ptr) LuaProcess(app);
    luaL_getmetatable(L, PROCES_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_process_delete(lua_State *L) {
    LuaProcess* process = luaL_checkprocess(L, 1);
    process->~LuaProcess();
    return 0;
}

static int lua_process_kill(lua_State *L) {
    LuaProcess* process = luaL_checkprocess(L, 1);
    int signum = luaL_checkint(L, 2);
    process->Kill(signum);
    return 0;
}

static int lua_process_close(lua_State *L) {
    LuaProcess* process = luaL_checkprocess(L, 1);
    process->Close();
    return 0;
}

static int lua_process_spawn(lua_State *L) {
    LuaProcess* process = luaL_checkprocess(L, 1);

    LuaState S{ L };
    process::ProcessOptions options;
    S >> options;

    int err = process->Spawn(options);
    if (err == 0) {
        return 0;
    }
    lua_pushstring(L, tinynet_strerror(err));
    return 1;
}

static int lua_process_on_event(lua_State *L) {
    auto process = luaL_checkprocess(L, 1);
    int type = lua_type(L, 2);
    luaL_argcheck(L, type == LUA_TNIL || type == LUA_TFUNCTION, 2, "function expected!");
    int nref = LUA_REFNIL;
    if (type == LUA_TFUNCTION) {
        lua_pushvalue(L, 2);
        nref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    process->set_event_callback(nref);
    return 0;
}

static int lua_process_get_pid(lua_State *L) {
    auto process = luaL_checkprocess(L, 1);
    lua_pushinteger(L, process->GetPid());
    return 1;
}

static const luaL_Reg process_methods[] = {
    { "new", lua_process_new},
    { 0, 0 }
};

static const luaL_Reg process_meta_methods[] = {
    { "spawn", lua_process_spawn},
    { "kill", lua_process_kill},
    { "close", lua_process_close},
    { "on_event", lua_process_on_event},
    { "get_pid", lua_process_get_pid},
    {"__gc", lua_process_delete },
    { 0, 0 }
};

LUALIB_API int luaopen_process(lua_State *L) {

    luaL_newmetatable(L, PROCES_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, process_meta_methods, 0);
    lua_pop(L, 1);

    luaL_newlib(L, process_methods);

    return 1;
}
