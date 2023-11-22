// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "raft_node.h"
#include "raft_service.h"
#include "util/random_utils.h"
#include "util/string_utils.h"
#include "logging/logging.h"
#include "rpc/rpc_helper.h"
#include "base/error_code.h"
#include <functional>
#include <algorithm>
#include "util/fs_utils.h"
#include "raft_peer.h"

#define TRACE_LOG(fmt, ...) this->Trace(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

namespace tinynet {
namespace raft {
static const int kElectionTimeout = 5000;

static const int kHeartbeatTimeout = 1000;

static const int kAppendEntriesCount = 128;

static const int kCompactEntriesCount = 100000;

static const size_t kInstallSnapshotFrameSize = 8 * 1024 * 1024;

static const char* STATE_NAMES[] = {
    "Unknown",
    "Leader",
    "Candidate",
    "Follower"
};

namespace internal {
template <class T>
T median_low(std::vector<T>& numbers) {
    if (numbers.size() == 0) {
        return 0;
    }
    if (numbers.size() == 1) {
        return numbers[0];
    }
    std::sort(numbers.begin(), numbers.end());
    if ((numbers.size() % 2) == 0) {
        return numbers[numbers.size() / 2 - 1];
    }
    return numbers[numbers.size() / 2];
}
}

RaftNode::RaftNode(RaftService *service, RaftStateMachine *state_machine) :
    commit_index_(0),
    last_applied_(0),
    current_time_(Time_ms()),
    role_(StateType::Unkown),
    event_loop_(service->event_loop()),
    service_(service),
    state_machine_(state_machine),
    leader_id_(0),
    election_timer_(INVALID_TIMER_ID),
    heartbeat_timer_(INVALID_TIMER_ID),
    votes_count_(0) {
    (void)service_;
}

RaftNode::~RaftNode() {
    if (election_timer_) {
        StopElectionTimer();
    }
    if (heartbeat_timer_) {
        StopHeartbeatTimer();
    }
}

int RaftNode::Init(const NodeConfig &config) {
    int err;
    config_ = config;
    config_.snapshotCount = config_.snapshotCount <= 0 ? kCompactEntriesCount : config_.snapshotCount;
    config_.heartbeatInterval = config_.heartbeatInterval <= 0 ? kHeartbeatTimeout : config_.heartbeatInterval;
    config_.electionTimeout = config_.electionTimeout <= 0 ? kElectionTimeout : config_.electionTimeout;
    err = ERROR_OK;
    if (config_.debugMode) {
        json::Document value;
        value << config_;
        TRACE_LOG("Init with config: %s", json::tojson(value).c_str());
    }
    log_manager_.reset(new (std::nothrow) RaftLogManager());
    if (!log_manager_) {
        err = ERROR_OS_OOM;
        return err;
    }

    bool result = FileSystemUtils::exists(config_.dataDir);
    if (!result) {
        FileSystemUtils::create_directories(config_.dataDir);
    }

    peers_.resize(config_.peers.size());
    for (size_t i = 0; i < config_.peers.size(); ++i) {
        peers_[i] = std::make_shared<RaftPeer>(event_loop_, static_cast<int>(i));
        peers_[i]->Init(config_.peers[i]);
    }

    if ((err = log_manager_->Init(config_.dataDir))) {
        return err;
    }

    Recover();

    Startup();

    return err;
}

void RaftNode::Stop() {
    for (auto peer : peers_) {
        peer->Close();
    }
    StopElectionTimer();
    StopHeartbeatTimer();
}

void RaftNode::Apply(std::vector<LogEntryPtr> &entries ) {
    for(auto &entry: entries) {
        ++last_applied_;
        if (state_machine_) {
            state_machine_->ApplyEntry(entry->index, entry->data);
        }
    }
}

void RaftNode::ApplyEntry(LogEntryPtr entry) {
    ++last_applied_;
    if (state_machine_) {
        state_machine_->ApplyEntry(entry->index, entry->data);
    }
}

LogEntryPtr RaftNode::NextEntry() {
    LogEntryPtr entry = std::make_shared<LogEntry>();
    entry->index = log_manager_->get_last_index() + 1;
    entry->term = log_manager_->get_current_term();
    return entry;
}

void RaftNode::AppendEntry(LogEntryPtr entry) {
    log_manager_->AppendEntries({ entry });
    SendAppendEntries();

    AppendEntriesResp msg;
    msg.set_success(true);
    msg.set_term(log_manager_->get_current_term());
    msg.set_matchindex(entry->index);
    AppendEntriesResponse(config_.id, ERROR_OK, &msg);
}

void RaftNode::Startup() {
    if (config_.standalong) {
        BecomeLeader();
    } else {
        if (config_.id == 0) {
            BecomeLeader();
        } else {
            BecomeFollower(); // When servers start up, they begin as followers
        }
    }
}

void RaftNode::VoteSelf() {
    log_manager_->set_vote_for(config_.id);
    votes_count_ = 1;

    VoteReq msg;
    msg.set_term(log_manager_->get_current_term());
    msg.set_candidateid(log_manager_->get_vote_for());
    auto entry = log_manager_->GetEntry(log_manager_->get_last_index());
    msg.set_lastlogindex(entry ? entry->index: 0);
    msg.set_lastlogterm(entry ? entry->term: 0);

    for (auto peer : peers_) {
        if (peer->get_id() == config_.id) {
            continue;
        }
        msg.set_peerid(peer->get_id());

        auto callback = std::bind(&RaftNode::RequestVoteResponse, this, peer->get_id(),
                                  std::placeholders::_1, std::placeholders::_2);
        peer->RequestVote(msg, callback);
    }
}

void RaftNode::ResetElectionTimer() {
    StopElectionTimer();
    int randomized_election_timeout = config_.electionTimeout + (int)RandomUtils::Random32(0, config_.electionTimeout);
    election_timer_ = event_loop_->AddTimer(randomized_election_timeout, 0, std::bind(&RaftNode::BecomeCandidate, this));
}

void RaftNode::StopElectionTimer() {
    if (election_timer_) {
        event_loop_->ClearTimer(election_timer_);
    }
}

void RaftNode::BecomeFollower() {
    ChangeRole(StateType::Follower);
    ResetElectionTimer();
    if (state_machine_) {
        state_machine_->StateChanged(role_);
    }
}

void RaftNode::BecomeCandidate() {
    if (is_leader()) return;
    ChangeRole(StateType::Candidate);	//and transitions to candidate state
    NewTerm();	//Increment currentTerm
    VoteSelf(); //Vote for self and Send RequestVote RPCs to all other servers
    ResetElectionTimer(); //Reset election timer
    if (state_machine_) {
        state_machine_->StateChanged(role_);
    }
}

void RaftNode::BecomeLeader() {
    ChangeRole(StateType::Leader);
    SetLeader(config_.id);
    ResetHeartbeatTimer();
    if (state_machine_) {
        state_machine_->StateChanged(role_);
    }
}

void RaftNode::Heartbeat() {
    if (!is_leader()) {
        StopHeartbeatTimer();
        return;
    }
    current_time_ = Time_ms();
    SendAppendEntries();
}

void RaftNode::ResetHeartbeatTimer() {
    StopHeartbeatTimer();
    heartbeat_timer_ = event_loop_->AddTimer(0, config_.heartbeatInterval, std::bind(&RaftNode::Heartbeat, this));
}

void RaftNode::StopHeartbeatTimer() {
    if (heartbeat_timer_) {
        event_loop_->ClearTimer(heartbeat_timer_);
    }
}

void RaftNode::SendAppendEntries() {
    if (config_.standalong) return;
    for (size_t i = 0; i < peers_.size(); ++i) {
        SendAppendEntries((int)i);
    }
}

void RaftNode::SendAppendEntries(int peerId) {
    if (peerId == config_.id) return;

    auto &peer = peers_[peerId];
    AppendEntriesReq msg;
    msg.set_term(log_manager_->get_current_term());
    msg.set_leaderid(get_id());
    msg.set_peerid(peerId);
    msg.set_leadercommit(commit_index_);

    uint64_t start_index = log_manager_->get_start_index();
    if (next_index_[peerId] > start_index) {
        auto prev_entry = log_manager_->GetEntry(next_index_[peerId] - 1);
        if (!prev_entry) return;
        msg.set_prevlogindex(prev_entry->index);
        msg.set_prevlogterm(prev_entry->term);
    } else {
        msg.set_prevlogindex(log_manager_->get_snapshot()->get_last_index());
        msg.set_prevlogterm(log_manager_->get_snapshot()->get_last_term());
    }
    uint64_t first = (std::max)(next_index_[peerId], log_manager_->get_start_index());
    uint64_t last = (std::min)(log_manager_->get_next_index(), first + kAppendEntriesCount);
    for (uint64_t index = first; index < last; ++index) {
        LogEntryPtr entry = log_manager_->GetEntry(index);
        if (!entry) return;
        auto entry_data = msg.add_entries();
        entry_data->set_index(entry->index);
        entry_data->set_term(entry->term);
        entry_data->set_data(entry->data);
    }
    auto callback = std::bind(&RaftNode::AppendEntriesResponse, this, peerId,
                              std::placeholders::_1, std::placeholders::_2);
    peer->AppendEntries(msg, callback);
}

void RaftNode::SendInstallSanpshot(int peerId) {
    if (peerId == config_.id) return;
    auto snap_file = log_manager_->get_snapshot()->get_snapshot_file();
    if (!snap_file) return;

    if (snap_index_[peerId] != log_manager_->get_snapshot()->get_last_index()) {
        snap_index_[peerId] = log_manager_->get_snapshot()->get_last_index();
        snap_offset_[peerId] = 0;
    }
    if (snap_offset_[peerId] >= snap_file->length()) return;

    auto &peer = peers_[peerId];
    InstallSnapshotReq  msg;
    msg.set_peerid(peerId);
    msg.set_term(log_manager_->get_current_term());
    msg.set_leaderid(get_id());
    msg.set_lastincludedindex(log_manager_->get_snapshot()->get_last_index());
    msg.set_lastincludedterm(log_manager_->get_snapshot()->get_last_term());
    const char* data = snap_file->data() + snap_offset_[peerId];
    size_t len = (std::min)(snap_file->length() - snap_offset_[peerId], kInstallSnapshotFrameSize);
    msg.set_offset(static_cast<uint32_t>(snap_offset_[peerId]));
    msg.set_data(data, len);
    snap_offset_[peerId] += len;
    msg.set_done(snap_offset_[peerId] >= snap_file->length());
    auto callback = std::bind(&RaftNode::InstallSnapshotResponse, this, peerId,
                              std::placeholders::_1, std::placeholders::_2);
    peer->InstallSnapshot(msg, std::move(callback));
    TRACE_LOG("Send install snapshot to peer %d", peerId);
}

void RaftNode::SetLeader(int id) {
    if (leader_id_ == id) return;
    leader_id_ = id;
    TRACE_LOG("Set leader %d", leader_id_);
}

void RaftNode::SetCurrentTime(int64_t current_time) {
    current_time_ = current_time;
}

void RaftNode::InitLeaderState() {
    next_index_.resize(config_.peers.size());
    match_index_.resize(config_.peers.size());
    snap_offset_.resize(config_.peers.size());
    snap_index_.resize(config_.peers.size());
    for (size_t i = 0; i < config_.peers.size(); ++i) {
        match_index_[i] = 0;
        next_index_[i] = log_manager_->get_next_index();
        snap_offset_[i] = 0;
        snap_index_[i] = 0;
    }
}

void RaftNode::InitCandidateState() {
    votes_count_ = 0;
    SetLeader(kNilNode);
}

void RaftNode::InitFollowerState() {
    log_manager_->set_vote_for(kNilNode);
}

bool RaftNode::ApplyTerm(uint64_t term) {
    if (term < log_manager_->get_current_term()) return false;

    if (term > log_manager_->get_current_term()) {
        log_manager_->set_current_term(term);
        if (!is_follower()) BecomeFollower();
    }
    return true;
}

void RaftNode::Recover() {
    // 1+ apply snapshot
    io::FileMapping* snap_file = log_manager_->get_snapshot()->get_snapshot_file();
    if (snap_file && state_machine_) {
        const char* data = snap_file->data() + sizeof(uint32_t);
        size_t len = snap_file->length() - sizeof(uint32_t);
        state_machine_->LoadSnapshot(data, len);
    }
    // 2+ apply log entries
    uint64_t first = log_manager_->get_start_index();
    uint64_t last = log_manager_->get_last_index();
    for (uint64_t index = first; index <= last; ++index) {
        ApplyEntry(log_manager_->GetEntry(index));
    }
    commit_index_ = last_applied_ = log_manager_->get_last_index();
}

void RaftNode::ChangeRole(StateType role) {
    if (role_ == role) return;
    auto old_role = role_;
    role_ = role;
    switch(role_) {
    case StateType::Unkown: {
        break;
    }
    case StateType::Leader: {
        InitLeaderState();
    }
    case StateType::Candidate: {
        InitCandidateState();
    }
    case StateType::Follower: {
        InitFollowerState();
    }
    default:
        break;
    }
    TRACE_LOG("Change role from %s to %s", STATE_NAMES[(int)old_role], STATE_NAMES[(int)role_]);
}

void RaftNode::NewTerm() {
    log_manager_->incr_current_term();
    TRACE_LOG("New term %lld", log_manager_->get_current_term());
}

void RaftNode::Commit(uint64_t leaderCommit) {
    commit_index_ = (std::min)(leaderCommit, log_manager_->get_last_index());

    if (commit_index_ > last_applied_) {
        uint64_t first = last_applied_ + 1;
        uint64_t last = commit_index_;
        for (uint64_t index = first; index <= last; ++index) {
            ApplyEntry(log_manager_->GetEntry(index));
            Compaction();
        }
    }
}

void RaftNode::Compaction() {
    uint64_t applied_count =  last_applied_ - log_manager_->get_start_index() + 1;
    if (applied_count < (uint64_t)config_.snapshotCount) {
        return;
    }
    IOBuffer buffer;
    if (state_machine_) {
        state_machine_->SaveSnapshot(&buffer);
    }
    auto entry = log_manager_->GetEntry(last_applied_);
    if (entry) {
        log_manager_->SaveSnapshot(entry->index, entry->term, buffer);
    }
}

void RaftNode::Trace(const char* file, int line, const char *fmt, ...) {
    if (!config_.debugMode) return;
    std::string msg;
    StringUtils::Format(msg, "[RAFT][ %s  %d ] ", STATE_NAMES[(int)role_], config_.id);

    va_list ap;
    va_start(ap, fmt);
    StringUtils::VFormat(msg, fmt, ap);
    va_end(ap);
    g_Logger->Log(file, line, 0, msg.c_str(), msg.length());
}

void RaftNode::RequestVote(::google::protobuf::RpcController* controller, const ::tinynet::raft::VoteReq* request, ::tinynet::raft::VoteResp* response, ::google::protobuf::Closure* done) {
    rpc::ClosureGuard done_guard(done);
    if (config_.standalong) {
        response->set_term(log_manager_->get_current_term());
        response->set_votegranted(true);
        return;
    }

    bool term_check = ApplyTerm(request->term());
    bool can_vote = log_manager_->get_vote_for() == kNilNode || log_manager_->get_vote_for() == request->candidateid();
    bool index_is_current = request->lastlogindex() >= log_manager_->get_last_index();
    bool granted = term_check && can_vote && index_is_current;
    if (granted) {
        log_manager_->set_vote_for(request->candidateid());
        ResetElectionTimer();
    }
    response->set_term(log_manager_->get_current_term());
    response->set_votegranted(granted);
    TRACE_LOG("Vote for candidate %d, granted:%s", request->candidateid(), granted ? "true" : "false");
}

void RaftNode::RequestVoteResponse(int peerId, int error_code, const ::tinynet::raft::VoteResp* response) {
    if (error_code) {
        TRACE_LOG("Vote response from peer %d failed with error[%d], msg[%s]",
                  peerId, error_code, tinynet_strerror(error_code));
        return;
    }
    ApplyTerm(response->term());
    if (response->votegranted()) {
        ++votes_count_;
    }
    TRACE_LOG("Node %d term %lld respond to vote, granted:%s", peerId, response->term(), response->votegranted() ? "true" : "false");
    if (votes_count_ > get_quorum()) {
        BecomeLeader();
    }
}

void RaftNode::AppendEntries(::google::protobuf::RpcController* controller, const ::tinynet::raft::AppendEntriesReq* request, ::tinynet::raft::AppendEntriesResp* response, ::google::protobuf::Closure* done) {
    rpc::ClosureGuard done_guard(done);
    if (config_.standalong) {
        response->set_term(log_manager_->get_current_term());
        response->set_success(true);
        return;
    }

    bool term_check = ApplyTerm(request->term());
    bool prev_term_check;
    if (request->prevlogindex() <= log_manager_->get_snapshot()->get_last_index()) {
        prev_term_check = true;
    } else {
        auto prev_entry = log_manager_->GetEntry(request->prevlogindex());
        prev_term_check = prev_entry && prev_entry->term == request->prevlogterm();
    }
    bool success = term_check && prev_term_check;
    if (term_check) {
        SetCurrentTime(request->timestamp());
        SetLeader(request->leaderid());
        if (is_follower()) {
            ResetElectionTimer();
        } else {
            BecomeFollower();
        }
    }
    if (success) {
        if (request->entries_size() > 0) {
            std::vector<LogEntryPtr> log_entries;
            for (int i = 0; i < request->entries_size(); ++i) {
                auto& entry_data = request->entries(i);
                if (entry_data.index() >= log_manager_->get_start_index()) {
                    auto entry = std::make_shared<LogEntry>();
                    entry->index = entry_data.index();
                    entry->term = entry_data.term();
                    entry->data = entry_data.data();
                    log_entries.emplace_back(entry);
                }
            }
            if (log_entries.size() > 0) {
                uint64_t first = log_entries.front()->index;
                if (first <= log_manager_->get_last_index()) {
                    log_manager_->EraseEntries(first, log_manager_->get_next_index());
                }
                log_manager_->AppendEntries(log_entries);
                Commit(request->leadercommit());
            }
        }

    } else {
        if (!term_check) {
            TRACE_LOG("Could not append entries. wrong term!");
        }
        if (!prev_term_check) {
            TRACE_LOG("Could not append entries. candidate's prev log entry(index:%lld, term:%lld) mismatch!", request->prevlogindex(), request->prevlogterm());
        }
    }
    response->set_term(log_manager_->get_current_term());
    response->set_success(success);
    response->set_matchindex(log_manager_->get_last_index());
}

void RaftNode::AppendEntriesResponse( int peerId, int error_code, const ::tinynet::raft::AppendEntriesResp *response) {
    if (error_code != ERROR_OK) return;

    if (response->success()) {
        match_index_[peerId] = response->matchindex();
        next_index_[peerId] = response->matchindex() + 1;
        int64_t index = internal::median_low(match_index_);
        Commit(index);
        return;
    }

    if (response->term() > log_manager_->get_current_term()) {
        ApplyTerm(response->term());
        return;
    }
    //If AppendEntries fails because of log inconsistency, decrement nextIndex and retry
    if (next_index_[peerId] > log_manager_->get_start_index()) {
        --next_index_[peerId];
        SendAppendEntries(peerId);
        return;
    }
    SendInstallSanpshot(peerId);
}

void RaftNode::InstallSnapshot(::google::protobuf::RpcController* controller, const ::tinynet::raft::InstallSnapshotReq* request,
                               ::tinynet::raft::InstallSnapshotResp* response, ::google::protobuf::Closure* done) {
    rpc::ClosureGuard done_guard(done);
    if (config_.standalong) {
        response->set_term(log_manager_->get_current_term());
        return;
    }
    bool term_check = ApplyTerm(request->term());
    if (!term_check) {
        response->set_term(log_manager_->get_current_term());
        return;
    }
    bool result = log_manager_->InstallSapshot(request->lastincludedindex(), request->lastincludedterm(),
                  request->offset(), request->data(), request->done());
    if (done && result) {
        Recover();
    }
    response->set_term(log_manager_->get_current_term());
}

void RaftNode::InstallSnapshotResponse(int peerId, int error_code, const ::tinynet::raft::InstallSnapshotResp *response) {
    if (response->term() > log_manager_->get_current_term()) {
        ApplyTerm(response->term());
    }
    SendInstallSanpshot(peerId);
}

}
}
