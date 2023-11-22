// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once

#include <string>
#include <memory>
#include "lua.hpp"
#include "zip.h"
#include "net/event_loop.h"
#include "app/app_container.h"

namespace tinynet {
class EventLoop;
class TimerManager;
namespace worker {
class WorkerService;
}
namespace lua {
struct LuaEnv {
    app::AppMeta		meta;
    app::AppContainer*	app{ nullptr };
};

class LuaScript  {
  public:
    LuaScript(app::AppContainer *app);
    ~LuaScript();
  public:
    void Init();
    void Update();
    void Stop();
    void Reload();
  public:
    int64_t guid() const { return guid_; }

    template<class T>
    T* get();
  private:
    void InitEnv();
    void OnApplicationQuit();
    void OnApplicationReload();
    static void* OnAlloc(void* ud, void *ptr, size_t osize, size_t nsize);
    static int OnPanic(lua_State* L);
  public:
    void Require(const std::string& name);
    void DoFile(const std::string& file_name);
    bool FileExists(const std::string& file_name);
    bool LoadFile(const std::string& file_name, std::string* file_content);
  private:
    int64_t     guid_;
    int64_t     taskid_;
    lua_State*  lua_state_;
    app::AppContainer *app_;
};
}
}
