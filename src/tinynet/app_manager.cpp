// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "app_manager.h"
#include <regex>
#include "util/string_utils.h"
#include "app/app_config.h"

using namespace tinynet;

MODULE_IMPL(AppManager);

int AppManager::Start() {
    event_loop_.reset(new tinynet::EventLoop());
    if (!event_loop_) return 1;
    event_loop_->Init();
    int ret;
    auto& app_metas = g_AppConfig->get_metas();
    for (size_t i = 0; i < app_metas.size(); ++i) {
        std::unique_ptr<app::AppContainer> app(new (std::nothrow) app::AppContainer((int64_t)(i + 1), app_metas[i]));
        if (!app) return 1;
        if ((ret = app->Init()) != 0) {
            return ret;
        }
        std::unique_ptr<std::thread> thread_ptr(new std::thread(&app::AppContainer::RunForever, app.get()));
        if (!thread_ptr) return 1;
        apps_.emplace_back(std::move(app));
        threads_.emplace_back(std::move(thread_ptr));
    }
    return 0;
}

int AppManager::Signal(int signum) {
    for (auto& app : apps_) {
        app->Signal(signum);
    }
    return 0;
}

int AppManager::Stop() {
    for (auto& app : apps_) {
        app->Exit();
    }
    for (auto& thread_ptr : threads_) {
        thread_ptr->join();
    }
    for (auto& app : apps_) {
        app->Stop();
    }
    if (event_loop_) event_loop_->Stop();
    return 0;
}

int AppManager::Run() {
    return event_loop_->Run(tinynet::EventLoop::RUN_ONCE);
}