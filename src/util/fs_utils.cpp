// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "fs_utils.h"
#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <dirent.h>
#endif
#include <stdint.h>
#include <string>
#include <string.h>
#include <algorithm>
#include <stdlib.h>
#include <list>
#include "string_utils.h"

#ifdef _WIN32
int symlink(const char *target, const char *linkpath) {
    std::wstring target_utf16, linkpath_utf16;

    if (StringUtils::convert_utf8_to_utf16(target, &target_utf16) == NULL ||
            StringUtils::convert_utf8_to_utf16(linkpath, &linkpath_utf16) == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (CreateSymbolicLinkW(linkpath_utf16.c_str(), target_utf16.c_str(), 0) == FALSE) {
        errno = GetLastError();
        return -1;
    }
    return 0;
}
#endif

namespace FileSystemUtils {

bool exists(const std::string &path) {
    return ACCESS(path.c_str(), 0) == 0;
}

bool create_directory(const std::string &path) {
    if (ACCESS(path.c_str(), 0) == 0) {
        return true;
    }
    return MKDIR(path.c_str()) == 0;
}

bool create_directories(const std::string &path) {
    if (path.length() == 0 || path.length() > PATH_MAX) {
        return false;
    }
    size_t pos = 0;
    while ((pos = path.find_first_of("/\\", ++pos)) != std::string::npos) {
        std::string part_of_path = path.substr(0, pos);
        if (!create_directory(part_of_path)) {
            return false;
        }
    }
    return create_directory(path);
}

bool is_directory(const std::string &path) {
    struct STAT buf;
    if (STAT(path.c_str(), &buf)) {
        return false;
    }
    return S_ISDIR(buf.st_mode);
}

bool is_file(const std::string &path) {
    struct STAT buf;
    if (STAT(path.c_str(), &buf)) {
        return false;
    }
    return S_ISREG(buf.st_mode);
}

void remove(const std::string& path) {
    UNLINK(path.c_str());
}

std::string& path_nomalize(std::string& path) {
    std::transform(path.begin(), path.end(), path.begin(), [](char ch) {
        return ch == '\\' ? '/' : ch;
    });
    return path;
}

std::string& path_join(std::string& dst, const std::string& basepath, const std::string& subpath) {
    const std::string* paths[2] = { &basepath, &subpath };
    for (auto path : paths) {
        if (dst.size() > 0 && dst.back() != '/' && dst.back() != '\\') {
            dst.append(1, '/');
        }
        if (path->size() > 0) {
            bool begin_with_slash = path->front() == '/' || path->front() == '\\';
            dst.append(path->begin() + (int)begin_with_slash, path->end());
        }
    }
    return dst;
}

std::string& path_join(std::string& dst, const std::string& basepath, const std::string& subpath, const std::string& extname) {
    return path_join(dst, basepath, subpath).append(extname);
}

std::string& path_windows_style_conversion(std::string& path) {
    std::transform(path.begin(), path.end(), path.begin(), [](char ch) {
        return ch == '/' ? '\\' : ch;
    });
    return path;
}

std::string basedir(const std::string& path) {
    size_t i = path.find_last_of("/\\");
    if (i != std::string::npos) {
        return path.substr(0, i + 1);
    }
    return "";
}

std::string basename(const std::string& path, bool keep_extension /* = true */) {
    std::string name;
    size_t i = path.find_last_of("/\\");
    if (i == std::string::npos) {
        name = path;
    } else {
        name = path.substr(i + 1);
    }
    if (!keep_extension && (i = name.find_last_of(".")) != std::string::npos) {
        name.erase(i);
    }
    return name;
}


std::string rootdir(const std::string& path) {
    size_t i = path.find_first_of("/\\");
    if (i != std::string::npos) {
        return path.substr(0, i);
    }
    return "";
}

bool is_absolute_path(const char* path) {
    if (path == NULL) {
        return false;
    }
#ifdef _WIN32
    if (strlen(path) >= 2) {
        if (path[0] == '\\' && path[1] == '\\') {
            return true;
        }
        if (isalpha(path[0]) && path[1] == ':') {
            return true;
        }
    }
#else
    if (path[0] == '/') {
        return true;
    }
#endif
    return false;
}

std::string fullpath(const std::string& path) {
    if (is_absolute_path(path.c_str())) {
        return path;
    }
    char buf[PATH_MAX] = { 0 };
    GETCWD(buf, sizeof(buf));
    std::string tmp_path;
    tmp_path.append(buf);
#ifdef _WIN32
    tmp_path.append(1, '\\');
#else
    tmp_path.append(1, '/');
#endif
    tmp_path.append(path);
    if (tmp_path.find("./") == std::string::npos &&
            tmp_path.find(".\\") == std::string::npos) {
        path_nomalize(tmp_path);
        return tmp_path;
    }
    size_t pos = 0;
    size_t begin = 0;
    std::list<std::string> arr;
    while ((pos = tmp_path.find_first_of("/\\", ++pos)) != std::string::npos) {
        std::string name = tmp_path.substr(begin, pos - begin);
        if (name == ".") {
        } else if (name == "..") {
            arr.pop_back();
        } else {
            arr.emplace_back(std::move(name));
        }
        begin = pos + 1;
    }
    arr.emplace_back(tmp_path.substr(begin));
    std::string res;
    size_t n = arr.size();
    for (auto& name : arr) {
        --n;
        res.append(name);
        if (n > 0)
            res.append(1, '/');
    }
    return res;
}

std::string relativepath(const std::string& root, const std::string& path) {
    if (root.empty()) {
        return path;
    }
    if (path.find(root) != 0) {
        return path;
    }
    if (root.length() >= path.length()) {
        return "";
    }
    size_t pos = root.length();
    if (root[root.length() - 1] != '/' && root[root.length() - 1] != '\\') {
        pos = root.length() + 1;
        if (path[root.length()] != '/' && path[root.length()] != '\\') {
            return path;
        }
    }
    return path.substr(pos);
}

static bool isslash(char ch) {
    return ch == '/' || ch == '\\';
}

size_t relativepath(const char* root, const char* path, char* buf, size_t len) {
    const char* pr = root;
    const char* pp = path;
    while (*pr != '\0' && *pp != '\0' && ((toupper(*pr) == toupper(*pp)) || (isslash(*pr) && isslash(*pp)))) {
        ++pr;
        ++pp;
    }
    if (isslash(*pp)) ++pp;
    size_t n = 0;

    char* pb = buf;
    while (*pp != '\0' && n < len - 1) {
        *pb++ = *pp++;
        ++n;
    }

    buf[n] = '\0';
    return n;
}

size_t basedir(const char* path, char* buf, size_t len) {
    size_t path_len = strlen(path);
    size_t i = 0; //store index of last '/' or '\\' character
    for (size_t j = 0; j < path_len; j++) {
        if (j < len - 1) {
            buf[j] = path[j];
            if (path[j] == '/' || path[j] == '\\') {
                i = j + 1;
            }
        } else {
            break;
        }
    }
    buf[i] = '\0';
    return i;
}

size_t basename(const char* path, char* buf, size_t len) {
    size_t i = 0;
    for (const char* p = path; p && (*p != '\0'); p++) {
        if (*p == '/' || *p == '\\') {
            i = 0;
        } else {
            if (i < len - 1) {
                buf[i++] = *p;
            } else {
                break;
            }
        }
    }
    buf[i] = '\0';
    return i;
}

size_t rootdir(const char* path, char* buf, size_t len) {
    size_t i = 0;
    for (const char* p = path; p && (*p != '\0'); p++) {
        if (*p == '/' || *p == '\\') {
            break;
        } else {
            if (i < len - 1) {
                buf[i++] = *p;
            } else {
                break;
            }
        }
    }
    buf[i] = '\0';
    return i;
}

static const char* dirent_type_names[] = {
    "unknown",	//0 FS_DT_UNKNOWN
    "fifo",		//1 FS_DT_FIFO
    "char",		//2 FS_DT_CHR
    "",	//3
    "dir",		//4 FS_DT_DIR
    "",	//5
    "block",	//6 FS_DT_BLK
    "",	//7
    "file",		//8 FS_DT_REG
    "",  //9
    "link",		//10 FS_DT_LNK
    "",	//11
    "socket",	//12 FS_DT_SOCK
    "",  //13
    "whiteout"  //14 FS_DT_WHT
};

#ifdef _WIN32
int readdir(const std::string& path, std::vector<DirectoryEntry>* output) {
    WIN32_FIND_DATAW find_data;
    std::wstring wpath;
    if (StringUtils::convert_utf8_to_utf16(path, &wpath) == NULL) {
        return -1;
    }
    if (wpath.find_last_of(L"/\\") == (wpath.length() - 1)) {
        wpath.append(1, L'*');
    } else {
        wpath.append(L"/*");
    }
    HANDLE hfile = FindFirstFileW(wpath.c_str(), &find_data);
    if (hfile == INVALID_HANDLE_VALUE) {
        return -1;
    }
    if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        FindClose(hfile);
        return -1;
    }
    while (FindNextFileW(hfile, &find_data)) {
        if ((find_data.cFileName[0] == L'.' && find_data.cFileName[1] == L'\0') ||
                (find_data.cFileName[0] == L'.' && find_data.cFileName[2] == L'\0')) {
            continue;
        }
        DirectoryEntry entry;
        StringUtils::convert_utf16_to_utf8(find_data.cFileName, &entry.name);
        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            entry.d_type = FS_DT_DIR;
        else if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
            entry.d_type = FS_DT_LNK;
        else if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) != 0)
            entry.d_type = FS_DT_CHR;
        else
            entry.d_type = FS_DT_REG;
        entry.type = dirent_type_names[entry.d_type];
        output->emplace_back(std::move(entry));
    }
    FindClose(hfile);
    return 0;
}
#else
int readdir(const std::string& path, std::vector<DirectoryEntry>* output) {
    DIR *dir;
    struct dirent *find_data;
    if ((dir = opendir(path.c_str())) == NULL) {
        return -1;
    }
    while ((find_data = readdir(dir)) != NULL) {
        if ((find_data->d_name[0] == '.' && find_data->d_name[1] == '\0') ||
                (find_data->d_name[0] == '.' && find_data->d_name[2] == '\0')) {
            continue;
        }
        DirectoryEntry entry;
        entry.name = find_data->d_name;
        entry.d_type = find_data->d_type;
        entry.type = dirent_type_names[entry.d_type];
        output->emplace_back(std::move(entry));
    }
    closedir(dir);
    return 0;
}
#endif

const char* dirent_typename(DirentType type) {
    return dirent_type_names[type];
}

std::string* get_cwd(std::string* path) {
    char buf[PATH_MAX];
    if (GETCWD(buf, sizeof(buf)) != NULL) {
        path->append(buf);
    }
    return path;
}

#ifdef _WIN32
size_t file_size(int fd) {
    return GetFileSize((HANDLE)_get_osfhandle(fd), NULL);
}
int file_truncate(int fd, int size) {
    return _chsize(fd, size);
}
#else
size_t file_size(int fd) {
    struct stat st;
    if (fstat(fd, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

int file_truncate(int fd, int size) {
    return ftruncate(fd, size);
}
#endif

void list_files(const std::string& path, const std::string& ext, std::vector<std::string>* output) {
    std::vector<DirectoryEntry> entries;
    readdir(path, &entries);
    for (auto& entry : entries) {
        if (entry.d_type == DirentType::FS_DT_REG ||
                entry.d_type == DirentType::FS_DT_UNKNOWN ||
                (ext.length() > 0 && StringUtils::EndsWith(entry.name, ext))) {
            output->push_back(entry.name);
        }
    }
    if (!output->empty()) {
        std::sort(output->begin(), output->end());
    }
}

}
