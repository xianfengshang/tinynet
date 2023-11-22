#include "log_destination.h"
#include "util/string_utils.h"
#include "util/fs_utils.h"
#include "base/console_logger.h"

namespace tinynet {
namespace logging {

bool FLAGS_logtostderr = true;

static const char* LOG_LEVEL_BASENAME[] = {
    "debug",
    "info",
    "warn",
    "error",
    "fatal",
};
static_assert(sizeof(LOG_LEVEL_BASENAME) / sizeof(LOG_LEVEL_BASENAME[0]) == MAX_LOG_LEVELS, "LOG_LEVEL_BASENAME does not match the definition of Logger::LogLevel");

static ConsoleColor LevelToColor(int level) {
    switch (level) {
    case LOG_LEVEL_DEBUG:
        return ConsoleColor::Default;
    case LOG_LEVEL_INFO:
        return ConsoleColor::Cyan;
    case LOG_LEVEL_WARN:
        return ConsoleColor::Yellow;
    case LOG_LEVEL_ERROR:
        return ConsoleColor::Red;
    case LOG_LEVEL_FATAL:
        return ConsoleColor::Magenta;
    default:
        return ConsoleColor::Default;
    }
}

LogDestination::LogDestination() {

}

LogDestination::~LogDestination() {
}

void LogDestination::Init(const std::string& path) {
    bool result = FileSystemUtils::exists(path);
    if (!result) {
        FileSystemUtils::create_directories(path);
    }
    std::string destination;
    for (int i = 0; i < MAX_LOG_LEVELS; ++i) {
        destination.clear();
        StringUtils::Format(destination, "%s/%s", path.c_str(), LOG_LEVEL_BASENAME[i]);
        main_destinations[i] = std::make_shared<LogFile>(destination);
    }
}

void LogDestination::LogToFile(int level, const char* message, size_t len) {
    if (level < 0 || level >= MAX_LOG_LEVELS)  return;
    if (!main_destinations[level]) return;
    main_destinations[level]->Append(message, len);
    LogToStderr(level, message, len);
}

void LogDestination::LogToFile(const std::string& base_filename, const char* message, size_t len) {
    auto it = custom_destinations.find(base_filename);
    if (it != custom_destinations.end()) {
        it->second->Append(message, len);
        return;
    }
    auto base_dir = FileSystemUtils::basedir(base_filename);
    bool result = FileSystemUtils::exists(base_dir);
    if (!result) {
        FileSystemUtils::create_directories(base_dir);
    }
    auto destination = std::make_shared<LogFile>(base_filename);
    custom_destinations[base_filename] = destination;
    destination->Append(message, len);
}

void LogDestination::LogToStderr(int level, const char* message, size_t len) {
    if (FLAGS_logtostderr) {
        g_ConsoleLogger->Log(message, len, LevelToColor(level));
    }
}

void LogDestination::Flush(int level) {
    for (int i = 0; i < MAX_LOG_LEVELS; ++i) {
        if (i >= level)
            main_destinations[i]->Flush(true);
    }
}

void LogDestination::Flush(const std::string& base_filename) {
    auto it = custom_destinations.find(base_filename);
    if (it == custom_destinations.end()) {
        return;
    }
    it->second->Flush(true);
}

void LogDestination::FlushAll() {
    for (int i = 0; i < MAX_LOG_LEVELS; ++i) {
        main_destinations[i]->Flush(true);
    }
    for (auto& it : custom_destinations) {
        it.second->Flush(true);
    }
}
}
}