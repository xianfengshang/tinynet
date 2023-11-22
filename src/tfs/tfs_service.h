// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "zip.h"
#include "tfs_types.h"
#include "base/io_buffer.h"
namespace tinynet {
namespace tfs {
class TfsService;
typedef std::shared_ptr<TfsService> TfsServicePtr;

/**  TinyNet File system service */
class TfsService {
  public:
    void Init();
    void Stop();
  public:
    bool Exists(const std::string& file_path);
    bool LoadFile(const std::string& file_path, std::string* file_content);
    bool LoadFile(const std::string& file_path, IOBuffer* file_content);
    bool Readdir(const std::string& dir_path, std::vector<DirectoryEntry>& output);
    void ClearCache();
  private:
    bool LoadPackage(const std::string& package_name);
    bool TryLoadPackage(const std::string& package_name);
    bool LookupPackage(const std::string& abs_path_hint);
    bool ExistsInPackage(const std::string& abs_file_path);
    bool ExistsInPackage(const std::string& abs_file_path, const std::string& package_name, zip_t* package);
    bool LoadFileFromDisk(const std::string& file_path, IOBuffer* file_content);
    bool LoadFileFormPackage(const std::string& abs_file_path, IOBuffer* file_content);
    bool LoadFileFormPackage(const std::string& abs_file_path, IOBuffer* file_content, const std::string& package_name, zip_t* package);
    bool ReaddirFromDisk(const std::string& dir_path, std::vector<DirectoryEntry>& output);
    bool ReaddirFromPackage(const std::string& abs_dir_path, std::vector<DirectoryEntry>& output);
    bool ReaddirFromPackage(const std::string& abs_dir_path, std::vector<DirectoryEntry>& output,
                            const std::string& package_name, zip_t* package);
  public:
    using zip_ptr_t = std::shared_ptr<zip_t>;
    static void ZipDeleter(void* ptr);
  private:
    std::unordered_map<std::string, zip_ptr_t> packages_;
};
}
}
