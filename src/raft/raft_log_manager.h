// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include "raft_snapshot.h"
#include "raft_memory_log.h"
#include "io/file_stream.h"
#include "wal.pb.h"
namespace tinynet {
namespace raft {
class RaftLogManager {
  public:
    int Init(const std::string& data_dir);
    void AppendEntries(const std::vector<LogEntryPtr>& entries);
    void EraseEntries(uint64_t first, uint64_t last);
    bool InstallSapshot(uint64_t index, uint64_t term, uint32_t offset, const std::string& data, bool done);
    void SaveSnapshot(uint64_t index, uint64_t term, IOBuffer& buffer);
  public:
    uint64_t get_start_index();
    uint64_t get_last_index();
    uint64_t get_next_index();
    std::shared_ptr<LogEntry> GetEntry(uint64_t index);
    size_t EntrySize();
    RaftSnapshot* get_snapshot() { return snapshot_.get(); }
  private:
    bool LoadWAL();
    bool LoadWAL(const std::string& filename, bool done);
    bool LoadRecord(const WALRecord& record);
    void AddRecord(const WALRecord& record);
    void LogRotate();
  public:
    uint64_t get_current_term() const { return current_term_; }
    void set_current_term(uint64_t term);
    uint64_t incr_current_term();

    int get_vote_for() const { return vote_for_; }
    void set_vote_for(int value);
  private:
    std::unique_ptr<RaftMemoryLog> log_;
    std::unique_ptr<RaftSnapshot> snapshot_;
    io::FileStreamPtr log_stream_;
    std::string data_dir_;
    std::string wal_dir_;
    uint64_t current_term_;
    int vote_for_;
    uint64_t wal_seq_;
};
}
}
