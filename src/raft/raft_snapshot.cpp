// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "raft_snapshot.h"
#include "raft_node.h"
#include "raft.pb.h"
#include "logging/logging.h"
#include "base/error_code.h"
#include "util/fs_utils.h"
#include "util/string_utils.h"
#include "io/file_stream.h"
#include "base/coding.h"
#include "base/io_buffer_stream.h"
#include <stdio.h>
namespace tinynet {
namespace raft {

static const char* SNAP_DIR_NAME = "snap";

static const char* SNAP_FILE_EXT = ".snap";

static const char* SNAP_NAME_FORMAT = "%016llu-%016llu.snap";

RaftSnapshot::RaftSnapshot():
    last_index_(0),
    last_term_(0) {
}

RaftSnapshot::~RaftSnapshot() = default;

int RaftSnapshot::Init(const std::string& data_dir) {
    FileSystemUtils::path_join(snap_dir_, data_dir, SNAP_DIR_NAME);
    if (FileSystemUtils::exists(snap_dir_)) {
        int err =  Load() ? ERROR_OK : ERROR_RAFT_SNAPSHOTLOADERROR;
        return err;
    }
    FileSystemUtils::create_directories(snap_dir_);
    return ERROR_OK;
}

io::FileMapping* RaftSnapshot::get_snapshot_file() {
    if (!snapshot_file_) {
        std::string filename, bin_path;
        StringUtils::Format(filename, SNAP_NAME_FORMAT, last_index_, last_term_);
        FileSystemUtils::path_join(bin_path, snap_dir_, filename);
        if (FileSystemUtils::exists(bin_path)) {
            Load(filename);
        }
    }
    return snapshot_file_.get();
}

bool RaftSnapshot::Load() {
    std::vector<std::string> files;
    FileSystemUtils::list_files(snap_dir_, SNAP_FILE_EXT, &files);
    if (files.empty()) {
        return true;
    }
    std::reverse(files.begin(), files.end());
    std::vector<std::string> obsolete_files;
    bool res = false;
    int err;
    for (auto& file : files) {
        if (res) {
            obsolete_files.push_back(file);
            continue;
        }
        err = Load(file);
        if (err == 0) {
            res = true;
        } else if (err < 0) {
            obsolete_files.push_back(file);
        }
    }
    for (auto& file: obsolete_files) {
        std::string path;
        FileSystemUtils::path_join(path, snap_dir_, file);
        FileSystemUtils::remove(path);
    }
    return res;
}

int RaftSnapshot::Load(const std::string& filename) {
    uint64_t index, term;
    if (sscanf(filename.c_str(), SNAP_NAME_FORMAT, &index, &term) != 2) {
        log_error("Unrecognized snapshot file %s", filename.c_str());
        return -1;
    }
    std::string path;
    FileSystemUtils::path_join(path, snap_dir_, filename);
    io::FileStreamPtr fp = io::FileStream::OpenReadable(path.c_str());
    if (!fp) return -1;

    char buf[sizeof(uint32_t)];
    if (fp->Length() < sizeof(buf)) {
        log_error("Load snapshot file %s faild, data corruption");
        return -1;
    }
    if (fp->Read(buf, sizeof(buf)) < sizeof(buf)) {
        log_error("Load snapshot file %s faild, read error");
        return -1;
    }
    uint32_t len = DecodeFixed32(buf);
    if (fp->Length() != len + sizeof(buf)) {
        log_error("Load snapshot file %s faild, length error");
        return -1;
    }
    std::unique_ptr <io::FileMapping> content(new (std::nothrow) io::FileMapping(fp->get_fd()));
    if (!content) {
        log_error("Load snapshot file %s faild, out of memory", filename.c_str());
        return 1;
    }
    if (!content->good()) {
        log_error("Load snapshot file %s faild, can not create content mapping", filename.c_str());
        return 1;
    }
    snapshot_file_.reset(content.release());
    last_index_ = index;
    last_term_ = term;
    return 0;
}

bool RaftSnapshot::Save(uint64_t index, uint64_t term, IOBuffer& data) {
    std::string filename, staging_path, bin_path;
    StringUtils::Format(filename, SNAP_NAME_FORMAT, index, term);
    FileSystemUtils::path_join(staging_path, snap_dir_, filename, ".staging");
    FileSystemUtils::path_join(bin_path, snap_dir_, filename);
    auto stream = io::FileStream::OpenWritable(staging_path.c_str());
    if (!stream) {
        log_error("Save snapshot failed, can not create staging file %s",
                  staging_path.c_str());
        return false;
    }
    char buf[sizeof(uint32_t)];
    EncodeFixed32(buf, (uint32_t)data.size());
    stream->Write(buf, sizeof(buf));
    stream->Write(data.begin(), data.size());
    stream.reset();
    if (rename(staging_path.c_str(), bin_path.c_str()) != 0) {
        log_error("Save snapshot failed, can not rename staging file %s to %s",
                  staging_path.c_str(), bin_path.c_str());
        return false;
    }
    snapshot_file_.reset();
    last_index_ = index;
    last_term_ = term;
    return true;
}

bool RaftSnapshot::Install(uint64_t index, uint64_t term, uint32_t offset, const std::string& data, bool done) {
    if (offset == 0) {
        std::string filename, staging_path;
        StringUtils::Format(filename, SNAP_NAME_FORMAT, index, term);
        FileSystemUtils::path_join(staging_path, snap_dir_, filename, ".staging");
        install_file_ = io::FileStream::OpenWritable(staging_path.c_str());
    }
    if (!install_file_) return false;

    install_file_->Seek(offset);
    install_file_->Write(&data[0], data.length());
    if (done) {
        std::string filename, staging_path, bin_path;
        StringUtils::Format(filename, SNAP_NAME_FORMAT, index, term);
        FileSystemUtils::path_join(staging_path, snap_dir_, filename, ".staging");
        FileSystemUtils::path_join(bin_path, snap_dir_, filename);
        install_file_.reset();
        if (rename(staging_path.c_str(), bin_path.c_str()) != 0) {
            log_error("Install snapshot failed, can not rename staging file %s to %s",
                      staging_path.c_str(), bin_path.c_str());
            return false;
        }
        snapshot_file_.reset();
        last_index_ = index;
        last_term_ = term;
    }
    return true;
}
}
}
