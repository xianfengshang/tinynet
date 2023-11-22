#pragma once
#include <string>
#include <unordered_map>
#include "logging.h"
#include "log_file.h"
#include "log_level.h"

namespace tinynet {
namespace logging {
class LogDestination {
  public:
    LogDestination();
    ~LogDestination();
  public:
    void Init(const std::string& path);
  public:
    void LogToFile(int level, const char* message, size_t len);
    void LogToFile(const std::string& base_filename, const char* message, size_t len);
    void LogToStderr(int level, const char* message, size_t len);
    void Flush(int level);
    void Flush(const std::string& base_filename);
    void FlushAll();
  private:
    LogFilePtr main_destinations[MAX_LOG_LEVELS];
    std::unordered_map<std::string, LogFilePtr> custom_destinations;
};
}
}
