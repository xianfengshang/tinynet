// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <chrono>
#include <time.h>
#include <stdint.h>
#ifdef _WIN32
/*struct tm *safe_localtime(const time_t *timep, struct tm *result);*/
#define safe_localtime(T, R) localtime_s((R), (T))
#else
/*struct tm *safe_localtime(const time_t *timep, struct tm *result);*/
#define safe_localtime(T, R) localtime_r((T), (R))
#endif
namespace DateUtils {

const char* DateString(int64_t timestamp);

const char* NowDateString();
}
