#include "log_file.h"
#include "util/sys_utils.h"
#include "util/process_utils.h"
#include "util/date_utils.h"
#include "util/fs_utils.h"
#include "base/base.h"
#include "base/clock.h"
#include "base/runtime_logger.h"

namespace tinynet {
namespace logging {

int FLAGS_logbufsecs = 0;
int FLAGS_time_rolling_secs = 86400;
int FLAGS_max_log_size = 128; //size in MB
bool FLAGS_stop_logging_if_full_disk = true;

LogFile::LogFile(const std::string& basename) :
    basename_(basename),
    hostname_(SysUtils::get_host_name()),
    pid_(std::to_string(ProcessUtils::get_pid())),
    bytes_written_(0),
    bytes_dirty_(0),
    size_based_rolling_time_(0),
    time_based_rolling_time_(0),
    next_flush_time_ms_(0),
    stop_writing_(false) {
}

LogFile::~LogFile() {
    Flush(true);
}

void LogFile::Append(const char* data, size_t len) {
    MaybeRollFile();

    if (!stream_) return;

    if (!stop_writing_) {
        errno = 0;
        stream_->Write(data, len);
        if (FLAGS_stop_logging_if_full_disk && errno == ENOSPC) {
            stop_writing_ = true;
            return;
        }
        bytes_written_ += len;
        bytes_dirty_ += len;
    } else {
        if (Time_ms() >= next_flush_time_ms_) {
            stop_writing_ = false;
        }
        return;
    }
    MaybeFlush(false);
}

void LogFile::Flush(bool force) {
    if (stream_ && bytes_dirty_ > 0) {
        MaybeFlush(force);
    }
}

void LogFile::MaybeFlush(bool force) {
    int64_t now = Time_ms();
    if (force || (now >= next_flush_time_ms_) || (bytes_dirty_ >= 1000000)) {
        next_flush_time_ms_ = now + FLAGS_logbufsecs * 1000;
        bytes_dirty_ = 0;
        stream_->Flush();
    }
}

void LogFile::MaybeRollFile() {
    time_t current_time = time(NULL);
    if (!stream_ || current_time >= (time_based_rolling_time_ + FLAGS_time_rolling_secs)) {
        time_based_rolling_time_ = ALIGN_UP(current_time, (time_t)FLAGS_time_rolling_secs) + FLAGS_time_rolling_secs;
        RollFile(current_time);
    }
    if (bytes_written_ >= (size_t)(FLAGS_max_log_size * 1024 * 1024 )&& current_time > size_based_rolling_time_) {
        size_based_rolling_time_ = current_time;
        RollFile(current_time);
    }
}

void LogFile::RollFile(time_t rollingtime) {
    bytes_written_ = 0;
    bytes_dirty_ = 0;
    std::string filename = NewRollingFileName(rollingtime);
    stream_ = io::FileStream::OpenWritable(filename.c_str());
    if (!stream_) {
        log_runtime_error("Create rolling file:%s failed, err:%s",filename.c_str(), strerror(errno));
        return;
    }

    std::string linkpath = basename_ + ".current";
    std::string target = FileSystemUtils::basename(filename);
    UNLINK(linkpath.c_str());
    symlink(target.c_str(), linkpath.c_str());
}

std::string LogFile::NewRollingFileName(time_t rollingtime) {
    std::string filename;
    filename.reserve(PATH_MAX);
    filename = basename_;
    char buf[32];
    struct tm tmval;
    safe_localtime(&rollingtime, &tmval);
    strftime(buf, sizeof(buf), ".%Y%m%d-%H%M%S.", &tmval);
    filename += buf;
    filename += SysUtils::get_host_name();
    filename += '.';
    filename += pid_;
    filename += ".log";
    return filename;
}
}
}
