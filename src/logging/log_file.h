#pragma once
#include <string>
#include <mutex>
#include <io/file_stream.h>
#include <base/io_buffer.h>

namespace tinynet {
namespace logging {

class LogFile;
typedef std::shared_ptr<LogFile> LogFilePtr;

class LogFile {
  public:
    LogFile(const std::string& basename);
    ~LogFile();
  public:
    void Append(const char* data, size_t len);
    void Flush(bool force = false);
  private:
    void MaybeFlush(bool force);
    void MaybeRollFile();
    void RollFile(time_t rollingtime);
    std::string NewRollingFileName(time_t rollingtime);
  private:
    std::string basename_;
    std::string hostname_;
    std::string pid_;
    io::FileStreamPtr stream_;
    size_t bytes_written_;
    size_t bytes_dirty_;
    time_t size_based_rolling_time_;
    time_t time_based_rolling_time_;
    int64_t next_flush_time_ms_;
    bool stop_writing_;
};

}
}
