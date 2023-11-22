// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "base/io_buffer.h"
#include "io/file_mapping.h"
#include "io/file_stream.h"

namespace tinynet {
namespace raft {
class RaftNode;
class RaftSnapshot;
typedef std::shared_ptr<RaftSnapshot> RaftSnapshotPtr;

class RaftSnapshot {
  public:
    RaftSnapshot();

    ~RaftSnapshot();
  public:
    int Init(const std::string& data_dir);
    bool Save(uint64_t index, uint64_t term, IOBuffer& data);
    bool Install(uint64_t index, uint64_t term, uint32_t offset, const std::string& data, bool done);
  public:
  public:
    uint64_t get_last_index() const { return last_index_; }

    uint64_t get_last_term() const { return last_term_; }

    bool empty() { return (bool)snapshot_file_; }

    io::FileMapping* get_snapshot_file();
  private:
    bool Load();
    int Load(const std::string& filename);
  private:
    using FileMappingPtr = std::unique_ptr<io::FileMapping>;
  private:
    std::string snap_dir_;
    FileMappingPtr snapshot_file_;
    io::FileStreamPtr install_file_;
    uint64_t last_index_;
    uint64_t last_term_;
};
}
}
