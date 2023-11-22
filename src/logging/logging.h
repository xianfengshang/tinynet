// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "log_level.h"
#include "logger.h"

namespace tinynet {
namespace logging {
extern int FLAGS_logbufsecs;
extern int FLAGS_max_log_size;
extern bool FLAGS_logtostderr;
extern int FLAGS_time_rolling_secs;
extern int FLAGS_minloglevel;
extern bool FLAGS_stop_logging_if_full_disk;
}


}
#define log_info(fmt, ...) g_Logger->Info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_warning(fmt, ...) g_Logger->Warning(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) g_Logger->Error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_fatal(fmt, ...) g_Logger->Fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_flush() g_Logger->Flush()
