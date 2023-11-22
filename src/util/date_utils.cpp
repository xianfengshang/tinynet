// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "date_utils.h"
#include "base/clock.h"
#include <math.h>

namespace DateUtils {

static thread_local char date_buffer[] = "0000-00-00 00:00:00.000";
static thread_local time_t last_seconds = 0;

const char* DateString(int64_t timestamp_ms) {
    time_t seconds = static_cast<time_t>(timestamp_ms / 1000);
    int milliseconds = static_cast<int>(timestamp_ms % 1000);
    if (seconds != last_seconds) {
        tm tm_val;
        safe_localtime(&seconds, &tm_val);
        snprintf(date_buffer, sizeof(date_buffer), "%04d-%02d-%02d %02d:%02d:%02d.",
                 tm_val.tm_year + 1900,
                 tm_val.tm_mon + 1,
                 tm_val.tm_mday,
                 tm_val.tm_hour,
                 tm_val.tm_min,
                 tm_val.tm_sec);
        last_seconds = seconds;
    }
    for (int i = 22; i > 19; i--) {
        if (milliseconds > 0) {
            date_buffer[i] = milliseconds % 10 + '0';
            milliseconds /= 10;
        } else {
            date_buffer[i] = '0';
        }

    }
    //snprintf(date_buffer + 19, 5, ".%03d", milliseconds);
    return date_buffer;
}

const char* NowDateString() {
    return DateString(tinynet::Time_ms());
}

}
