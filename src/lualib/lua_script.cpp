// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_script.h"
#include "logging/logging.h"
#include "lualib/lua_helper.h"
#include "lualib/lua_tinynet.h"
#include "lua_proto_types.h"
#include "util/fs_utils.h"
#include "tfs/tfs_service.h"

#if !defined(LUA_PATH)
#define LUA_PATH    "LUA_PATH"
#endif

#define LUA_PATH_VALUE  \
		";;"	"./lua/?.lua;"  "./lua/?/init.lua;"

namespace tinynet {
namespace lua {

static const int kSIGHUP = 1;

LuaScript::LuaScript(app::AppContainer *app) :
    guid_(app->event_loop()->NewUniqueId()),
    taskid_(INVALID_TASK_ID),
    lua_state_(0),
    app_(app) {
}

LuaScript::~LuaScript() {
    if (lua_state_) {
        lua_close(lua_state_);
        lua_state_ = 0;
    }
}

template<> lua_State* LuaScript::get() { return lua_state_; }

template<> app::AppContainer* LuaScript::get() { return app_; }

template<> EventLoop* LuaScript::get() { return app_->event_loop(); }

void LuaScript::Init() {
#ifndef _WIN32
    if (setenv(LUA_PATH, LUA_PATH_VALUE, 1) == -1) {
        log_warning("setenv(%s, %s, 1) faild.", LUA_PATH, LUA_PATH_VALUE);
    }
#endif
    lua_state_ = lua_newstate(OnAlloc, this);
    if (!lua_state_) {
        log_fatal("luaL_newstate failed, maybe out of memory");
        return;
    }
    lua_atpanic(lua_state_, OnPanic);
    lua_setapp(lua_state_, app_);
    luaL_openlibs(lua_state_);
    luaL_opentinynet(lua_state_);

    InitEnv();

    taskid_ =  app_->event_loop()->AddSignal(kSIGHUP, std::bind(&LuaScript::Reload, this));
}

void LuaScript::Update() {
}

void LuaScript::Stop() {
    if (taskid_ != INVALID_TASK_ID) {
        app_->event_loop()->ClearSignal(kSIGHUP, taskid_);
        taskid_ = INVALID_TASK_ID;
    }
    OnApplicationQuit();
}

void LuaScript::Reload() {
    OnApplicationReload();
}

void LuaScript::InitEnv() {
    LuaEnv env;
    env.meta = app_->meta();
    env.app = app_;
    LuaState S{ lua_state_ };
    S << env;
    lua_setglobal(lua_state_, "env");
}

void LuaScript::OnApplicationQuit() {
    lua_getglobal(lua_state_, "OnApplicationQuit");
    luaL_pcall(lua_state_, 0, 0);
}

void LuaScript::OnApplicationReload() {
    lua_getglobal(lua_state_, "OnApplicationReload");
    luaL_pcall(lua_state_, 0, 0);
}

void LuaScript::DoFile(const std::string& file_name) {
    int top = lua_gettop(lua_state_);
    int result;
    std::string file_content;
    if (!app_->get<tfs::TfsService>()->LoadFile(file_name, &file_content)) {
        log_error("File %s not found", file_name.c_str());
        return;
    }
    result = luaL_loadbuffer(lua_state_, &file_content[0], file_content.size(), file_name.c_str()) || lua_pcall(lua_state_, 0, LUA_MULTRET, 0);
    if (result) {
        log_error("lua loadbuffer error: %s", lua_isstring(lua_state_, -1) ? lua_tostring(lua_state_, -1) : "");
        lua_pop(lua_state_, 1);
        return;
    }
    lua_settop(lua_state_, top);
}

void LuaScript::Require(const std::string& name) {
    int top = lua_gettop(lua_state_);
    lua_getglobal(lua_state_, "require");
    if (lua_type(lua_state_, -1) != LUA_TFUNCTION) {
        log_error("lua missing require function");
        lua_pop(lua_state_, 1);
        return;
    }
    lua_pushstring(lua_state_, name.c_str());
    luaL_pcall(lua_state_, 1, 1);
    lua_settop(lua_state_, top);
}

bool LuaScript::FileExists(const std::string& file_name) {
    return app_->get<tfs::TfsService>()->Exists(file_name);
}

bool LuaScript::LoadFile(const std::string& file_name, std::string* file_content) {
    return app_->get<tfs::TfsService>()->LoadFile(file_name, file_content);
}

void* LuaScript::OnAlloc(void* ud, void *ptr, size_t osize, size_t nsize) {
    LuaScript* self = (LuaScript*)ud;
    (void)self;
    (void)osize;
    if (nsize == 0) {
        free(ptr);
        return NULL;
    } else {
        return realloc(ptr, nsize);
    }
}

int LuaScript::OnPanic(lua_State *L) {
    const char* msg = lua_tostring(L, -1);
    if (msg == NULL) {
        msg = "unknown error";
    }
    log_error("[LUAVM] PANIC: %s", msg);
    return 0;
}
}
}
