// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <vector>
#include <stdio.h>
#include "base/string_view.h"

#ifdef _WIN32
#define ACCESS(fileName,accessMode) _access(fileName,accessMode)
#define MKDIR(path) _mkdir(path)
#define STAT _stat
#define S_ISREG(m) (((m) & _S_IFREG) == _S_IFREG)
#define S_ISDIR(m) (((m) & _S_IFDIR) == _S_IFDIR)
#define PATH_MAX  _MAX_PATH
#define GETCWD _getcwd
#define UNLINK _unlink
#define FILENO _fileno
extern int symlink(const char *target, const char *linkpath);
#else
#include <unistd.h>
#include <linux/limits.h>
#define ACCESS(fileName,accessMode) access(fileName,accessMode)
#define MKDIR(path) mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#define STAT stat
#define GETCWD getcwd
#define UNLINK unlink
#define FILENO fileno
#endif

namespace FileSystemUtils {
bool exists(const std::string &path);
bool create_directory(const std::string& path);
bool create_directories(const std::string &path);
bool is_directory(const std::string &path);
bool is_file(const std::string &path);
void remove(const std::string& path);
std::string& path_nomalize(std::string& path);
std::string& path_join(std::string& dst, const std::string& basepath, const std::string& subpath);
std::string& path_join(std::string& dst, const std::string& basepath, const std::string& subpath,
                       const std::string& extname);
std::string& path_windows_style_conversion(std::string& path);

std::string basedir(const std::string& path);
std::string basename(const std::string& path, bool keep_extension = true);
std::string rootdir(const std::string& path);
std::string fullpath(const std::string& path);
std::string relativepath(const std::string& root, const std::string& path);
size_t basedir(const char* path, char* buf, size_t len);
size_t basename(const char* path, char* buf, size_t len);
size_t rootdir(const char* path, char* buf, size_t len);
bool is_absolute_path(const char* path);
size_t relativepath(const char* root, const char* path, char* buf, size_t len);

enum DirentType {
    FS_DT_UNKNOWN = 0,
    FS_DT_FIFO = 1,
    FS_DT_CHR = 2,
    FS_DT_DIR = 4,
    FS_DT_BLK = 6,
    FS_DT_REG = 8,
    FS_DT_LNK = 10,
    FS_DT_SOCK = 12,
    FS_DT_WHT = 14
};

struct DirectoryEntry {
    std::string name;
    tinynet::string_view type;
    int d_type{ FS_DT_UNKNOWN };
};

int readdir(const std::string& path, std::vector<DirectoryEntry>* output);

const char* dirent_typename(DirentType type);

std::string* get_cwd(std::string* path);

size_t file_size(int fd);

int file_truncate(int fd, int size);

void list_files(const std::string& path,  const std::string& ext,
                std::vector<std::string>* output);
}
