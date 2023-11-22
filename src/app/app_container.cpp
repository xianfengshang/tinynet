// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "app_container.h"
#include <algorithm>
#include "util/string_utils.h"
#include "util/fs_utils.h"
#include "logging/logging.h"
#include "lualib/lua_script.h"
#include "net/http/http_client.h"
#include "cluster/cluster_service.h"
#include "tfs/tfs_service.h"

namespace tinynet {
namespace app {

AppContainer::AppContainer(int64_t id, const AppMeta &meta) :
    id_(id),
    meta_(meta) {
}

AppContainer::~AppContainer() = default;

template<> EventLoop* AppContainer::get() { return event_loop_.get(); }

template<> cluster::ClusterService* AppContainer::get() { return cluster_.get(); }

template<> http::HttpClient* AppContainer::get() { return http_.get(); }

template<> lua::LuaScript* AppContainer::get() { return script_.get(); }

template<> tfs::TfsService* AppContainer::get() { return tfs_.get(); }

template<> lua_State* AppContainer::get() { return script_->get<lua_State>(); }

int AppContainer::Init() {
    event_loop_.reset(new(std::nothrow) EventLoop());
    if (!event_loop_) return 1;

    int ret = event_loop_->Init();
    if (ret != 0) return ret;

    event_loop_->SetIdAllocator(new(std::nothrow) IdAllocator(id_ & IdAllocator::MaxworkerId_, id_ >> IdAllocator::workerId_Bits));

    tfs_.reset(new(std::nothrow) tfs::TfsService());
    if (!tfs_) return 1;

    tfs_->Init();
    cluster_.reset(new(std::nothrow) cluster::ClusterService(event_loop_.get()));
    if (!cluster_) return 1;

    http_.reset(new(std::nothrow) http::HttpClient(event_loop_.get()));
    if (!http_) return 1;

    http_->Init();
    script_.reset(new(std::nothrow) lua::LuaScript(this));
    if (!script_) return 1;

    script_->Init();
    if (FileSystemUtils::exists("script")) { //Adapt to early versions
        std::string filename;
        StringUtils::Format(filename, "script/%s/init.lua", meta_.name.c_str());
        script_->DoFile(filename);
    } else {
        script_->Require(meta_.name);
    }
    return 0;
}

void AppContainer::Stop() {
    cluster_->Stop();
    http_->Stop();
    script_->Stop();
    tfs_->Stop();
    event_loop_->Stop();
    event_loop_->Run(EventLoop::RUN_NOWAIT); //final run
}


int AppContainer::Run() {
    return event_loop_->Run(EventLoop::RUN_ONCE);
}

int AppContainer::RunForever() noexcept {
    return  event_loop_->Run();
}

void AppContainer::Exit() {
    event_loop_->Exit();
}

void AppContainer::Signal(int signum) {
    event_loop_->Signal(signum);
}


}
}
