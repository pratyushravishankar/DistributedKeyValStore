#pragma once

#include "Shards.h"
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "hashmap.pb.h"
#include "hashmap.grpc.pb.h"
#include "PartitionedHashMap.h"
#include "IExecutor.h"


constexpr int HEARTBEAT_INTERVAL = 150;  // in milliseconds
// constexpr int ELECTION_TIMEOUT = 300;   // Randomized timeout in milliseconds

enum class RaftState { FOLLOWER, CANDIDATE, LEADER };

struct LogEntry {
    int term;            // The term when the entry was received
    std::string key;     // Key for the operation
    std::string value;   // Value for the operation (if applicable)
    std::string opType;  // "PUT" or "ERASE"
};

struct RaftNodeState {
    RaftState state = RaftState::FOLLOWER;
    int currentTerm = 0; // The latest term the node has seen
    std::string votedFor; // Candidate this node voted for in the current term
    std::vector<LogEntry> log; // Log of entries (key, value, operation)
    int commitIndex = -1; // Last committed log index
    int lastApplied = -1; // Last applied log index
};

template<typename StubManager_>
class RaftNode {
public:

    RaftNode(const std::string& serverName, const std::string& shardName, const std::vector<std::string>& peers, const StubManager_& stubManager, std::shared_ptr<PartitionedHashMap> map, bool isTest, std::shared_ptr<Shards> shards, std::shared_ptr<IExecutor> executor, std::function<int()> electionTimeoutGenerator, bool useLocks = true)
        : mServerName{serverName}, mShardName{shardName}, mPeers{peers}, mState{}, mVoteCount{0}, mStubManager{stubManager}, mLocalMap{map}, mShards{shards}, mExecutor{executor}, mElectionTimeoutGenerator{std::move(electionTimeoutGenerator)}
    {
        mLastHeartbeatTime = mExecutor->currentTime();

        
        if (!isTest) {
            resetHeartbeatTimer();
            startElectionTimeout();
        }


    }


    void startElectionTimeout() {
        mElectionTimeout = std::chrono::milliseconds{mElectionTimeoutGenerator()};
        mExecutor->schedule(Task{TaskMeta{.serverName = mServerName, .taskName = "election_timeout"}, [this]() {
            onElectionTimeout();
        }}, std::chrono::milliseconds(mElectionTimeout));
    }

    void onElectionTimeout() {
        if (mUseLocks) std::lock_guard<std::mutex> lock(mMutex);

        if (mState.state == RaftState::LEADER) {
            return;
        }

        // Check elapsed time since last heartbeat
        auto elapsed = mExecutor->currentTime() - mLastHeartbeatTime;
        std::cout << "ELAPSED: " << elapsed << std::endl;
        if (elapsed >= mElectionTimeout) {
            std::cout << mServerName << ": Election timeout expired, starting election..." << std::endl;
            startElection();
        } else {
            std::cout << mServerName << ": Received heartbeat recently, no election." << std::endl;
        }
        startElectionTimeout();  // Reschedule the election timeout
    }

    // TODO only used in unittest - remove?
    void receiveHeartbeat(int term, const std::string& leader) {
        if (mUseLocks) std::lock_guard<std::mutex> lock(mMutex);

        if (term >= mState.currentTerm) {
            mState.currentTerm = term;
            mState.state = RaftState::FOLLOWER;
            mLeader = leader;
            resetHeartbeatTimer();
        }
    }

void resetHeartbeatTimer() {
    mLastHeartbeatTime = mExecutor->currentTime();
    std::cout << mServerName << ": Heartbeat timer reset." << std::endl;
}




    void startElection() {
        std::cout << mServerName << " starting election" << std::endl;

        mState.state = RaftState::CANDIDATE;
        mState.currentTerm++;
        mVoteCount = 1; // Vote for self

        for (const auto& peer : mPeers) {
            
            mExecutor->execute(Task{TaskMeta{.serverName = mServerName, .taskName = "requesting_vote"} ,[this, peer]() {
                hashmap::RequestVoteRequest request;
                request.set_term(mState.currentTerm);
                request.set_candidate_name(mServerName);
                request.set_last_log_index(mState.log.size() - 1);
                request.set_last_log_term(mState.log.empty() ? 0 : mState.log.back().term);

                hashmap::RequestVoteResponse response;
                grpc::ClientContext context;

                auto stub = mStubManager.getStub(peer);
                auto status = stub->RequestVote(&context, request, &response);

                if (status.ok() && response.vote_granted()) {
                    if (mUseLocks) std::lock_guard<std::mutex> lock(mMutex);
                    mVoteCount++;
                    if (mVoteCount > mPeers.size() / 2 && mState.state == RaftState::CANDIDATE) {
                        mState.state = RaftState::LEADER;
                        std::cout << mServerName << " is now the leader for term " << mState.currentTerm << std::endl;
                        mShards->updateLeader(mShardName, mServerName);
                        startHeartbeat();
                    }
                }
            }});
        }
    }

    void startHeartbeat() {
        if (mState.state == RaftState::LEADER) {
            // Send an immediate heartbeat
            for (const auto& peer : mPeers) {
                mExecutor->execute(Task{TaskMeta{.serverName = mServerName, .taskName = "heartbeat"}, [this, peer]() {
                    hashmap::AppendEntriesRequest request;
                    request.set_term(mState.currentTerm);
                    request.set_leader_name(mServerName);

                    hashmap::AppendEntriesResponse response;
                    grpc::ClientContext context;

                    auto stub = mStubManager.getStub(peer);
                    std::cout << "sending immediate heartbeat: " << mServerName << " --> " << peer << std::endl;
                    stub->AppendEntries(&context, request, &response);
                }});
            }

            // Schedule the next heartbeat
            mExecutor->schedule(Task{TaskMeta{.serverName = mServerName, .taskName = "heartbeat"}, [this]() {
                startHeartbeat();
            }}, std::chrono::milliseconds(HEARTBEAT_INTERVAL));
        }
    }



    ::grpc::Status RequestVote(::grpc::ServerContext* context,
                               const hashmap::RequestVoteRequest* request,
                               hashmap::RequestVoteResponse* response) {
        if (mUseLocks) std::lock_guard<std::mutex> lock(mMutex);

        if (request->term() < mState.currentTerm) {
            response->set_vote_granted(false);
            return ::grpc::Status::OK;
        }

        if (request->term() > mState.currentTerm) {
            mState.currentTerm = request->term();
            mState.state = RaftState::FOLLOWER;
            mState.votedFor.clear();
        }   
        // Ensure that the node is not a candidate voting for someone else
        if (mState.state == RaftState::CANDIDATE) {
            response->set_vote_granted(false);
            std::cout << "Rejecting vote because current node is a candidate" << std::endl;
            return ::grpc::Status::OK;
        }

        if (mState.votedFor.empty() && isUpToDate(request->last_log_index(), request->last_log_term())) {
            mState.votedFor = request->candidate_name();
            response->set_vote_granted(true);
        } else {
            response->set_vote_granted(false);
        }

        return ::grpc::Status::OK;
    }

    bool isUpToDate(int lastLogIndex, int lastLogTerm) {
        int myLastLogTerm = mState.log.empty() ? 0 : mState.log.back().term;
        if (lastLogTerm > myLastLogTerm) return true;
        if (lastLogTerm == myLastLogTerm && lastLogIndex >= mState.log.size() - 1) return true;
        return false;
    }

    void replicateLogToFollowers() {
        for (const auto& peer : mPeers) {
            mExecutor->execute(Task{TaskMeta{.serverName = mServerName, .taskName = "replicate_log"}, [this, peer]() {
                hashmap::AppendEntriesRequest request;
                request.set_term(mState.currentTerm);
                request.set_leader_name(mServerName);

                for (int i = mNextIndex[peer]; i < mState.log.size(); i++) {
                    const auto& e = mState.log[i];
                    hashmap::LogEntry entry;
                    entry.set_term(e.term);
                    entry.set_key(e.key);
                    entry.set_value(e.value);
                    entry.set_optype(e.opType);
                    *request.add_entries() = entry;
                }

                hashmap::AppendEntriesResponse response;
                grpc::ClientContext context;

                auto stub = mStubManager.getStub(peer);
                auto status = stub->AppendEntries(&context, request, &response);

                if (status.ok() && response.success()) {
                    mMatchIndex[peer] = mState.log.size() - 1;
                    mNextIndex[peer] = mState.log.size();
                } else if (status.ok()) {
                    mNextIndex[peer]--; // Retry with earlier log index
                }
            }});
        }
    }

    ::grpc::Status AppendEntries(::grpc::ServerContext* context, 
                                 const hashmap::AppendEntriesRequest* request, 
                                 hashmap::AppendEntriesResponse* response) {
        if (mUseLocks) std::lock_guard<std::mutex> lock(mMutex);

        if (request->term() < mState.currentTerm) {
            response->set_success(false);
            return ::grpc::Status::OK;
        }

        resetHeartbeatTimer();
        // Update term and leader    
        mState.currentTerm = request->term();
        mState.state = RaftState::FOLLOWER;
        mLeader = request->leader_name();

        std::cout << "recevied heartbeat from " << mLeader << std::endl;
        // Append new entries to log
        for (const auto& entry : request->entries()) {
            LogEntry e {
                .term = entry.term(),
                .key = entry.key(),
                .value = entry.value(),
                .opType = entry.optype()
            };
            appendToLog(e);
        }

        

        // Update commit index if needed
        if (request->commit_index() > mState.commitIndex) {
            // std::abort();
            mState.commitIndex = std::min(request->commit_index(), (int)mState.log.size() - 1);
            applyCommittedEntries();
        }

        response->set_success(true);
        return ::grpc::Status::OK;
    }

    void appendToLog(const LogEntry& entry) {
        mState.log.push_back(entry);
    }

    void applyCommittedEntries() {
        while (mState.lastApplied < mState.commitIndex) {
            mState.lastApplied++;
            const auto& entry = mState.log[mState.lastApplied];
            if (entry.opType == "PUT") {
                mLocalMap->insert(entry.key, entry.value);
            } else if (entry.opType == "ERASE") {
                mLocalMap->erase(entry.key);
            }
        }
    }

    void tryCommitLogs() {
        if (mUseLocks) std::lock_guard<std::mutex> lock(mMutex);

        // Determine the highest index replicated to a majority of followers
        int majority = (mPeers.size() + 1) / 2;  // +1 to include the leader
        for (int i = mState.commitIndex + 1; i < mState.log.size(); ++i) {
            int replicatedCount = 1;  // Leader always has its own log
            for (const auto& [peer, matchIndex] : mMatchIndex) {
                if (matchIndex >= i) replicatedCount++;
            }
            if (replicatedCount > majority) {
                mState.commitIndex = i;  // Update commitIndex
            } else {
                break;  // No need to check further; indices are ordered
            }
        }

        // Apply newly committed entries to the local state machine
        applyCommittedEntries();
    }


    bool isLeader() {
        return mState.state == RaftState::LEADER;
    }

    std::string getLeader() {
        return mLeader;
    }



    const RaftNodeState& getState() {
        return mState;
    }

    auto& getLocalMap() const{
        return *mLocalMap;
    }



private:
    std::string mServerName;
    std::string mShardName;
    std::vector<std::string> mPeers;
    std::unordered_map<std::string, int> mNextIndex;
    std::unordered_map<std::string, int> mMatchIndex;
    RaftNodeState mState;
    int mVoteCount;
    std::string mLeader;
    std::mutex mMutex;
    const StubManager_& mStubManager;


    std::chrono::milliseconds  mLastHeartbeatTime; // Tracks last heartbeat time
    std::atomic<bool> mRunning;                          // To control the election timeout thread
    std::chrono::milliseconds mElectionTimeout;     

    // struct LocalMap {
    //     void insert(const std::string& key, const std::string& value) {
    //         committed[key] = value;
    //     };
    //     void erase(const std::string& key) {
    //         committed.erase(key);
    //     };
        
    //     std::unordered_map<std::string, std::string> committed;

    //     auto& get() {
    //         return committed;
    //     }
    // };
    // LocalMap mLocalMap;
    std::shared_ptr<PartitionedHashMap> mLocalMap;
    std::shared_ptr<Shards> mShards;
    std::shared_ptr<IExecutor> mExecutor;
    std::function<int()> mElectionTimeoutGenerator;
    bool mUseLocks;
};