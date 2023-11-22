// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "base/application.h"
#include "app_init.h"
#include "base/cmdline.h"
#include "logging/logging.h"
#include "base/io_buffer_stream.h"
#include "app/app_config.h"
#include "util/string_utils.h"
#include "base/winsock_manager.h"
#include "base/error_code.h"
#include "base/runtime_logger.h"

const static size_t kOpenFilesLimit = 65535;

MODULE_IMPL(AppInit);

AppInit::AppInit() {
#ifdef _WIN32
    g_WinsockManager->Init();
#endif
}
AppInit::~AppInit() {
#ifdef _WIN32
    g_WinsockManager->Shutdown();
#endif
}
int AppInit::Init() {
#include "ascii_logo.h"
    ParseCommandLineArgs();
    int err = g_Logger->Init(g_App->GetArgs()[0], log_dir_);
    if (err) {
        log_runtime_error("g_Logger->Init failed, err:%s", tinynet_strerror(err));
        return -1;
    }
    if (g_Logger->IsServerMode())
        g_Logger->Log(ascii_logo, strlen(ascii_logo));
    auto cmdline = StringUtils::join(g_App->GetArgs(), " ");
    log_info("c++ version:%ul, build time: %s, %s", __cplusplus, __DATE__, __TIME__);
    log_info(cmdline.c_str());
    if (g_AppConfig->Init(app_names_, app_labels_)) {
        log_error("g_ServiceConfig->InitService failed!");
        return -1;
    }
#ifdef __linux__
    LinuxMemoryWarning();
    AdjustOpenFilesLimit();
#endif
    return 0;
}

int AppInit::Stop() {
    g_Logger->Shutdown();
    return 0;
}

void AppInit::ParseCommandLineArgs() {
    tinynet::cmdline::Parser cmd_parser;
    cmd_parser.add("log", 'l', "specify log file output directory");
    cmd_parser.add("app", 'a', "specify running server apps");
    cmd_parser.add("labels", 'L', "specify attribute labels for app", tinynet::cmdline::Parser::OptionType::kv,
                   tinynet::cmdline::Parser::OptionRule::optional);

    cmd_parser.parse_check(g_App->GetArgs());
    log_dir_ = cmd_parser.get("log");
    app_names_ = cmd_parser.get("app");
    app_labels_ = cmd_parser.get("labels");
}

#ifdef __linux__
#include "util/sys_utils.h"
void AppInit::LinuxMemoryWarning() {
    if (SysUtils::transparent_hugepage_is_enabled()) {
        log_warning("Transparent Huge Pages (THP) should be disabled for best performance.");
    }
}

void AppInit::AdjustOpenFilesLimit() {
    if (!SysUtils::adjust_open_files_limit(kOpenFilesLimit)) {
        log_warning("Adjust open files limit(ulimit -n) faild, please check and do it manually.");
        return;
    }
}
#endif
