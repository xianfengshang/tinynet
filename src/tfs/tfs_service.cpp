// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "tfs_service.h"
#include "util/fs_utils.h"
#include "util/string_utils.h"
#include "util/zip_utils.h"
#include "logging/logging.h"

namespace tinynet {
namespace tfs {

void TfsService::ZipDeleter(void* ptr) {
    zip_close(static_cast<zip_t*>(ptr));
}

void TfsService::Init() {
}

void TfsService::Stop() {
    packages_.clear();
}

bool TfsService::Exists(const std::string& file_path) {
    if (FileSystemUtils::exists(file_path)) {
        return true;
    }
    std::string abs_path = FileSystemUtils::fullpath(file_path);
    bool result = ExistsInPackage(abs_path);
    return result;
}

bool TfsService::LoadFile(const std::string& file_path, std::string* file_content) {
    IOBuffer io_buf;
    if (LoadFile(file_path, &io_buf)) {
        file_content->append(io_buf.begin(), io_buf.size());
        return true;
    }
    return false;
}

bool TfsService::LoadFile(const std::string& file_path, IOBuffer* file_content) {
    if (FileSystemUtils::exists(file_path)) {
        return LoadFileFromDisk(file_path, file_content);
    }
    std::string abs_path = FileSystemUtils::fullpath(file_path);
    if (ExistsInPackage(abs_path)) {
        return LoadFileFormPackage(abs_path, file_content);
    }
    return false;
}

bool TfsService::Readdir(const std::string& dir_path, std::vector<DirectoryEntry>& output) {
    if (FileSystemUtils::exists(dir_path)) {
        return ReaddirFromDisk(dir_path, output);
    }
    std::string abs_path = FileSystemUtils::fullpath(dir_path);
    if (!StringUtils::EndWith(abs_path, "/\\")) {
        abs_path.push_back('/');
    }
    if (LookupPackage(abs_path)) {
        return ReaddirFromPackage(abs_path, output);
    }
    return false;
}

bool TfsService::LoadPackage(const std::string& package_name) {
    if (!FileSystemUtils::is_file(package_name)) {
        return false;
    }
    int err;
    auto package = zip_open(package_name.c_str(), 0, &err);
    if (!package) {
        ZipUtils::zip_error_guard_t error(err);
        log_error("Load script package faild, name:%s, msg:%s", package_name.c_str(), error.what());
        return  false;
    }
    zip_ptr_t package_ptr(package, ZipDeleter);
    return packages_.emplace(package_name, package_ptr).second;
}

bool TfsService::TryLoadPackage(const std::string& package_name) {
    if (!FileSystemUtils::is_file(package_name)) {
        return false;
    }
    int err;
    auto package = zip_open(package_name.c_str(), 0, &err);
    if (!package) {
        return  false;
    }
    zip_ptr_t package_ptr(package, ZipDeleter);
    return packages_.emplace(package_name, package_ptr).second;
}

bool TfsService::LookupPackage(const std::string& abs_path_hint) {
    size_t pos = 0;
    bool result = false;
    while ((pos = abs_path_hint.find_first_of("/\\", ++pos)) != std::string::npos) {
        std::string part_of_path = abs_path_hint.substr(0, pos);
        if ((result = (packages_.find(part_of_path) != packages_.end()))) {
            break;
        }
        if ((result = TryLoadPackage(part_of_path))) {
            break;
        }
    }
    return result;
}

bool TfsService::ExistsInPackage(const std::string& abs_file_path) {
    if (!LookupPackage(abs_file_path)) {
        return false;
    }
    for (auto& pair : packages_) {
        if (ExistsInPackage(abs_file_path, pair.first, pair.second.get())) {
            return true;
        }
    }
    return false;
}

bool TfsService::ExistsInPackage(const std::string& abs_file_path, const std::string& package_name, zip_t* package) {
    if (abs_file_path.length() < package_name.length()) {
        return false;
    }
    size_t i = abs_file_path.find(package_name);
    if (i == std::string::npos) {
        return false;
    }
    std::string file_name = FileSystemUtils::basename(package_name) + abs_file_path.substr(i + package_name.length());
    FileSystemUtils::path_nomalize(file_name);
    return zip_name_locate(package, file_name.c_str(), 0) >= 0;
}

bool TfsService::LoadFileFromDisk(const std::string& file_path, IOBuffer* file_content) {
    FILE * fd = fopen(file_path.c_str(), "rb");
    if (fd == NULL) {
        return false;
    }
    fseek(fd, 0, SEEK_END);
    size_t size = ftell(fd);
    rewind(fd);
    if (size > 0) {
        file_content->resize(size);
        fread(file_content->begin(), sizeof(char), size, fd);
    }
    fclose(fd);
    return true;

}

bool TfsService::LoadFileFormPackage(const std::string& abs_file_path, IOBuffer* file_content) {
    for (auto& pair : packages_) {
        if (LoadFileFormPackage(abs_file_path, file_content, pair.first, pair.second.get())) {
            return true;
        }
    }
    return false;
}

bool TfsService::LoadFileFormPackage(const std::string& abs_file_path, IOBuffer* file_content, const std::string& package_name, zip_t* package) {
    size_t i = abs_file_path.find(package_name);
    if (i == std::string::npos) {
        return false;
    }
    std::string file_name = FileSystemUtils::basename(package_name) + abs_file_path.substr(i + package_name.length());
    FileSystemUtils::path_nomalize(file_name);
    struct zip_stat zstat;
    zip_stat_init(&zstat);
    if (zip_stat(package, file_name.c_str(), 0, &zstat) != 0) {
        log_error("Get information about file %s in package faild, msg:%s",
                  file_name.c_str(), zip_error_strerror(zip_get_error(package)));
        return false;
    }

    if (zstat.size == 0) {
        return true;
    }
    struct zip_file* file = zip_fopen(package, file_name.c_str(), 0);
    if (!file) {
        log_error("Open file %s in package faild", file_name.c_str());
        return false;
    }
    file_content->resize(zstat.size);

    auto len = zip_fread(file, file_content->begin(), zstat.size);
    if (len < 0) {
        file_content->resize(0);
        zip_fclose(file);
        log_error("Read content of file %s in package faild", file_name.c_str());
        return false;
    }

    zip_fclose(file);
    return true;
}

bool TfsService::ReaddirFromDisk(const std::string& dir_path, std::vector<DirectoryEntry>& output) {
    return FileSystemUtils::readdir(dir_path, &output) == 0;
}

bool TfsService::ReaddirFromPackage(const std::string& abs_dir_path, std::vector<DirectoryEntry>& output) {
    for (auto& pair : packages_) {
        if (ReaddirFromPackage(abs_dir_path, output, pair.first, pair.second.get())) {
            return true;
        }
    }
    return false;
}

void TfsService::ClearCache() {
    packages_.clear();
}

static void CollectDirectoryEntry(const std::string& dir_name, zip_stat_t* zs, std::vector<DirectoryEntry>& output) {
    std::string dst_name(zs->name);
    if (dst_name.length() <= dir_name.length()) {
        return;
    }
    if (dst_name.find(dir_name) != 0) {
        return;
    }
    //if (FileSystemUtils::basedir(dst_name) != dir_name) {
    //    return;
    //}
    DirectoryEntry entry;
    if (StringUtils::EndWith(dst_name, "/\\") && zs->size == 0) {
        //subdirectory
        entry.d_type = FileSystemUtils::FS_DT_DIR;
        entry.name = FileSystemUtils::relativepath(dir_name, dst_name);
    } else {
        //file
        entry.d_type = FileSystemUtils::FS_DT_REG;
        entry.name = FileSystemUtils::relativepath(dir_name, dst_name);
    }
    entry.type = FileSystemUtils::dirent_typename((FileSystemUtils::DirentType)entry.d_type);
    output.emplace_back(std::move(entry));
}

bool TfsService::ReaddirFromPackage(const std::string& abs_dir_path, std::vector<DirectoryEntry>& output, const std::string& package_name, zip_t* package) {
    size_t i = abs_dir_path.find(package_name);
    if (i == std::string::npos) {
        return false;
    }
    std::string dir_name = FileSystemUtils::basename(package_name) + abs_dir_path.substr(i + package_name.length());
    //if (zip_name_locate(package, dir_name.c_str(), 0) < 0) {
    //    return false;
    //}
    zip_stat_t zs;
    auto entries = zip_get_num_entries(package, 0);
    for (zip_int64_t n = 0; n < entries; ++n) {
        if (zip_stat_index(package, n, 0, &zs) == 0) {
            CollectDirectoryEntry(dir_name, &zs, output);
        }
    }
    return true;
}
}
}
