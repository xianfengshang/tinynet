// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "raft_log_manager.h"
#include "logging/logging.h"
#include "wal/log_recorder.h"
#include "util/fs_utils.h"
#include "util/string_utils.h"
#include "raft_types.h"
#include "google/protobuf/text_format.h"
#include "base/error_code.h"

namespace tinynet {
namespace raft {

static const char* WAL_FILE_EXT = ".wal";

static const char* WAL_NAME_FORMAT = "%016llu-%016llu.wal";

static const size_t WAL_FILE_SIZE_LIMIT = 64 * 1024 * 1024;

int RaftLogManager::Init(const std::string& data_dir) {
    int err = ERROR_OK;
    snapshot_.reset(new(std::nothrow) RaftSnapshot());
    if (!snapshot_) {
        err = ERROR_OS_OOM;
        return err;
    }
    if ((err = snapshot_->Init(data_dir))) {
        return err;
    }
    log_.reset(new (std::nothrow) RaftMemoryLog());
    if (!log_) {
        err = ERROR_OS_OOM;
        return err;
    }
    log_->reset(snapshot_->get_last_index() + 1);
    FileSystemUtils::path_join(wal_dir_, data_dir, "wal");
    if (!FileSystemUtils::exists(wal_dir_)) {
        FileSystemUtils::create_directories(wal_dir_);
    }
    current_term_ = 0;
    vote_for_ = kNilNode;
    if (!LoadWAL()) {
        err = ERROR_RAFT_WALLOADERROR;
        return err;
    }
    return err;
}

void RaftLogManager::EraseEntries(uint64_t first, uint64_t last) {
    WALRecord record;
    record.set_type(LT_EntriesErased);
    auto data = record.mutable_entries_erased();
    data->set_first_index(first);
    data->set_last_index(last);
    AddRecord(record);

    log_->erase(first, last);
}

void RaftLogManager::AppendEntries(const std::vector<LogEntryPtr>& entries) {
    WALRecord record;
    record.set_type(LT_EntriesAppended);
    auto data = record.mutable_entries_appended();
    for (auto& entry : entries) {
        auto entry_data = data->add_entries();
        entry_data->set_index(entry->index);
        entry_data->set_term(entry->term);
        entry_data->set_data(entry->data);
    }
    AddRecord(record);

    log_->append(entries);
}

bool RaftLogManager::InstallSapshot(uint64_t index, uint64_t term, uint32_t offset, const std::string& data, bool done) {
    bool result = snapshot_->Install(index, term, offset, data, done);
    if (done && result) {
        log_->reset(index + 1);
        LogRotate();
    }
    return result;
}

void RaftLogManager::SaveSnapshot(uint64_t index, uint64_t term, IOBuffer& buffer) {
    bool result = snapshot_->Save(index, term, buffer);
    if (result) {
        log_->reset(index + 1);
        LogRotate();
    }
}

uint64_t RaftLogManager::get_start_index() {
    return log_->begin();
}

uint64_t RaftLogManager::get_last_index() {
    return log_->end() - 1;
}

uint64_t RaftLogManager::get_next_index() {
    return log_->end();
}

std::shared_ptr<LogEntry> RaftLogManager::GetEntry(uint64_t index) {
    return log_->at(index);
}

size_t RaftLogManager::EntrySize() {
    return log_->size();
}

bool RaftLogManager::LoadWAL() {
    std::vector<std::string> files;
    FileSystemUtils::list_files(wal_dir_, WAL_FILE_EXT, &files);
    if (files.size() == 0) return true;

    std::vector<std::string> loading_files, obsolete_files;
    uint64_t start_index =  snapshot_->get_last_index();
    uint64_t seq, index, last_seq;
    seq = index = last_seq = 0;
    for (auto& file : files) {
        if (sscanf(file.c_str(), WAL_NAME_FORMAT, &seq, &index) != 2) {
            log_warning("Unrecognized wal file:%s", file.c_str());
            continue;
        }
        if (index > start_index) {
            if (loading_files.size() > 0) {
                if (seq != last_seq + 1) {
                    log_error("WAL file:%s is not continuous", file.c_str());
                    return false;
                }
            }
            loading_files.push_back(file);
            last_seq = seq;
        } else {
            obsolete_files.push_back(file);
        }
    }
    for (size_t i = 0; i < loading_files.size(); ++i) {
        const std::string& file = loading_files[i];
        bool res = LoadWAL(file, i == loading_files.size() - 1);
        if (!res) {
            log_error("Load wal file:%s error", file.c_str());
            return false;
        }
    }
    for (auto& file : obsolete_files) {
        std::string file_path;
        FileSystemUtils::path_join(file_path, wal_dir_, file);
        FileSystemUtils::remove(file_path);
    }
    return true;
}

bool RaftLogManager::LoadWAL(const std::string& filename, bool done) {
    uint64_t seq, index;
    if (sscanf(filename.c_str(), WAL_NAME_FORMAT, &seq, &index) != 2) {
        log_error("Unrecognized snapshot file %s", filename.c_str());
        return false;
    }
    std::string path;
    FileSystemUtils::path_join(path, wal_dir_, filename);
    io::FileStreamPtr log_stream;
    if (done) {
        log_stream = io::FileStream::OpenAppendable(path.c_str());
    } else {
        log_stream = io::FileStream::OpenReadable(path.c_str());
    }
    wal::LogRecorder reader(log_stream);
    WALRecord record;
    std::string data;
    while (reader.Next(&data)) {
        if (!record.ParseFromString(data)) {
            log_error("Parse wal record error");
            return false;
        }
        if (!LoadRecord(record)) {
            log_error("Load wal record error");
            return false;
        }
        data.clear();
    }
    if (reader.has_error()) {
        if (done) {
            log_stream->Truncate(reader.bytes_read());
            log_warning("Load wal file:%s data corruption, truncate it", filename.c_str());
        } else {
            log_error("Load wal file:%s faild, data corruption", filename.c_str());
            return false;
        }
    }
    if (done) {
        wal_seq_ = seq;
        log_stream_ = log_stream;
    }
    return true;
}

bool RaftLogManager::LoadRecord(const WALRecord& record) {
    switch (record.type()) {
    case LT_ConfChanged: {
        break;
    }
    case LT_TermChanged: {
        current_term_ = record.term_changed().current_term();
        break;
    }
    case LT_VoteChanged: {
        vote_for_ = record.vote_changed().voted_for();
        break;
    }
    case LT_EntriesAppended: {
        std::vector<LogEntryPtr> entries;
        for (int i = 0; i < record.entries_appended().entries_size(); ++i) {
            auto& entry_data = record.entries_appended().entries(i);
            LogEntryPtr entry = std::make_shared<LogEntry>();
            entry->index = entry_data.index();
            entry->term = entry_data.term();
            entry->data = entry_data.data();
            entries.emplace_back(std::move(entry));
        }
        log_->append(entries);
        break;
    }
    case LT_EntriesErased: {
        uint64_t first, last;
        first = record.entries_erased().first_index();
        last = record.entries_erased().last_index();
        log_->erase(first, last);
        break;
    }
    default:
        break;
    }
    return true;
}

void RaftLogManager::AddRecord(const WALRecord& record) {
    if (!log_stream_ || log_stream_->Length() >= WAL_FILE_SIZE_LIMIT) {
        LogRotate();
    }
    if (!log_stream_) return;
    wal::LogRecorder writer(log_stream_);
    auto msg = record.SerializeAsString();
    writer.Put(msg);
    log_stream_->Flush();
}

void RaftLogManager::LogRotate() {
    std::string name, path;
    StringUtils::Format(name, WAL_NAME_FORMAT, ++wal_seq_, log_->begin());
    FileSystemUtils::path_join(path, wal_dir_, name);
    log_stream_ = io::FileStream::OpenAppendable(path.c_str());
    if (!log_stream_) {
        log_error("Rotate wal faild, can not open file:%s for append", path.c_str());
    }
}

void RaftLogManager::set_current_term(uint64_t term) {
    if (current_term_ == term) return;
    WALRecord record;
    record.set_type(LT_TermChanged);
    auto data = record.mutable_term_changed();
    data->set_current_term(term);
    AddRecord(record);
    current_term_ = term;
}

uint64_t RaftLogManager::incr_current_term() {
    set_current_term(current_term_ + 1);
    return current_term_;
}

void RaftLogManager::set_vote_for(int value) {
    if (vote_for_ == value) return;
    WALRecord record;
    record.set_type(LT_VoteChanged);
    auto data = record.mutable_vote_changed();
    data->set_voted_for(value);
    AddRecord(record);
    vote_for_ = value;
}
}
}
