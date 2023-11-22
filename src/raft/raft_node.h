// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "net/event_loop.h"
#include "raft_types.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "raft_state_machine.h"
#include "raft_snapshot.h"
#include "raft.pb.h"
#include "raft_log_manager.h"

namespace tinynet {
namespace raft {

using LogEntryPtr = std::shared_ptr<LogEntry>;
class RaftService;
class RaftPeer;
using RaftPeerPtr = std::shared_ptr<RaftPeer>;

class RaftNode {
  public:
    RaftNode(RaftService *service, RaftStateMachine *state_machine);
    ~RaftNode();
  public:
    int Init(const NodeConfig &config);
    void Stop();
    int64_t Time() { return current_time_; }
    LogEntryPtr NextEntry();
    void AppendEntry(LogEntryPtr entry);
  private:
    void Apply(std::vector<LogEntryPtr> &entries);
    void ApplyEntry(LogEntryPtr entry);
  public:
    void Commit(uint64_t leaderCommit);
  public:
    void RequestVote(::google::protobuf::RpcController* controller, const ::tinynet::raft::VoteReq* request,
                     ::tinynet::raft::VoteResp* response,::google::protobuf::Closure* done);
    void RequestVoteResponse(int peerId, int error_code, const ::tinynet::raft::VoteResp* response);
    void AppendEntries(::google::protobuf::RpcController* controller, const ::tinynet::raft::AppendEntriesReq* request,
                       ::tinynet::raft::AppendEntriesResp* response,  ::google::protobuf::Closure* done);
    void AppendEntriesResponse(int peerId, int error_code, const ::tinynet::raft::AppendEntriesResp *response);
    void InstallSnapshot(::google::protobuf::RpcController* controller, const ::tinynet::raft::InstallSnapshotReq* request,
                         ::tinynet::raft::InstallSnapshotResp* response, ::google::protobuf::Closure* done);
    void InstallSnapshotResponse(int peerId, int error_code, const ::tinynet::raft::InstallSnapshotResp *response);
  private:
    void Startup();
    void VoteSelf();
    void BecomeFollower();
    void BecomeLeader();
    void BecomeCandidate();
    void ResetElectionTimer();
    void StopElectionTimer();
    void Heartbeat();
    void ResetHeartbeatTimer();
    void StopHeartbeatTimer();
    void SendAppendEntries();
    void SendAppendEntries(int peerId);
    void SendInstallSanpshot(int peerId);
    void SetLeader(int id);
    void SetCurrentTime(int64_t current_time);
    void InitLeaderState();
    void InitCandidateState();
    void InitFollowerState();
    //Rules for servers
    bool ApplyTerm(uint64_t term);
    void Compaction();
    void Recover();
    void ChangeRole(StateType role);
    void NewTerm();
  public:
    void Trace(const char* file, int line, const char *fmt, ...);
  public:
    int get_id() const { return config_.id; }

    bool is_leader() const { return role_ == StateType::Leader; }

    bool is_candidate() const { return role_ == StateType::Candidate; }

    bool is_follower() const { return role_ == StateType::Follower; }

    bool has_leader() const { return leader_id_ != kNilNode; }

    EventLoop * event_loop() { return event_loop_; }

    const std::string& get_leader_address() const { return config_.peers[leader_id_]; }

    size_t get_quorum() const { return config_.peers.size() / 2; }

    int64_t last_applied() const { return last_applied_; }
  private:
//Persistent state on all servers:
    //uint64_t current_term_;
    //int		voted_for_;

//Volatile state on all servers:
    uint64_t commit_index_;
    uint64_t last_applied_;
    int64_t current_time_;

//Volatile state on leaders
    std::vector<uint64_t> next_index_;
    std::vector<uint64_t> match_index_;
    std::vector<size_t> snap_offset_;
    std::vector<uint64_t> snap_index_;
    std::vector<RaftPeerPtr> peers_;

    NodeConfig config_;
    StateType role_;
    EventLoop * event_loop_;
    RaftService * service_;
    RaftStateMachine * state_machine_;
    int leader_id_;
    int64_t election_timer_;
    int64_t heartbeat_timer_;
    size_t votes_count_;

    std::unique_ptr<RaftLogManager> log_manager_;
};
}
}
