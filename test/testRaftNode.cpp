#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "Shards.h" // just for Shards struct // TOO remove once refactoed
#include "RaftNode.h"
#include "IExecutor.h"
#include <memory>
#include <algorithm>


#include "hashmap.pb.h"
#include "hashmap.grpc.pb.h"
#include <grpcpp/grpcpp.h>

using grpc::ClientContext;
using grpc::Status;
using hashmap::RequestVoteRequest;
using hashmap::RequestVoteResponse;
using hashmap::AppendEntriesRequest;
using hashmap::AppendEntriesResponse;

// TODO call function call on server's object, so grpc calls are completely mocked
struct MockStubManager {    
    struct MockStub {
        Status RequestVote(ClientContext* context, const RequestVoteRequest& request, RequestVoteResponse* response) {
            response->set_vote_granted(true);
            return Status::OK;
        }

        Status AppendEntries(ClientContext* context, const AppendEntriesRequest& request, AppendEntriesResponse* response) {
            response->set_success(true);
            return Status::OK;
        }
    };
    auto getStub(const std::string&) const {
        return std::make_unique<MockStub>(*stub);
    }
private:
   std::unique_ptr<MockStub> stub{std::make_unique<MockStub>()};
};


class TestExecutor : public IExecutor {
public:
    void schedule(Task task, std::chrono::milliseconds delay) override {
        const auto& meta = task.first;
        std::cout << "{ " << meta << " } scheduling task to do in " << delay << "ms" << std::endl;
        
        mTasks.push_back(TaskItem{mCurrentTime + delay, task});
        // Note: The current time is not updated when schedule() is called,
        // nor when execute() is called. This design choice simplifies reasoning
        // about time advancement in tests, as it removes the time taken to
        // execute different lines of code from affecting the current time.
    }

    void execute(Task task) override {
        mTasks.push_back(TaskItem{mCurrentTime, task});
        // Note: Similar to schedule(), the current time is not updated here,
        // allowing for a clearer understanding of time progression during tests.
    }

    std::chrono::milliseconds currentTime() const override{
        return mCurrentTime;
    }
    


    // Simulate the passage of time and execute due tasks
    void advanceTime(std::chrono::milliseconds duration) {
        mCurrentTime += duration;

        // Step 1: Collect tasks to execute
        std::vector<Task> tasksToExecute;
        for (auto it = mTasks.begin(); it != mTasks.end();) {
            if (it->executeAt <= mCurrentTime) {
                tasksToExecute.push_back(it->task);
                it = mTasks.erase(it);
            } else {
                ++it;
            }
        }
        // // Step 2: Execute collected tasks
        for (const auto& [meta, task] : tasksToExecute) {
            std::cout << "{ " << meta << " } executing... " << std::endl;
            task();
        }
    }


private:
    struct TaskItem {
        std::chrono::milliseconds executeAt;
        Task task;
    };

    // insert in queue by execution-time 
    // find position using upperbound to ensure all tasks with same exeuction time already in the queue are invoked before
    void insertTaskInQueue(TaskItem&& task) {
        auto it = std::ranges::upper_bound(mTasks, task.executeAt, {}, &TaskItem::executeAt);
        mTasks.insert(it, std::move(task));
    }

    struct TaskItemCompare {
        bool operator()(const TaskItem a, const TaskItem b) {
            return a.executeAt < b.executeAt;
        }
    };
    
    // std::priority_queue<TaskItem, std::vector<TaskItem>,  TaskItemCompare> mTasks;
    std::vector<TaskItem> mTasks;
    std::chrono::milliseconds mCurrentTime = std::chrono::milliseconds(0); // Start at 0 for controlled simulation

};


class RaftNodeTest : public ::testing::Test {
protected:
    RaftNodeTest()
        : mMockStub{std::make_shared<MockStubManager>()},
          mExecutor{std::make_shared<TestExecutor>()},
          mRaftNode{"leader", "shardA", {"peerA", "peerB"}, *mMockStub, 
                    std::make_shared<PartitionedHashMap>(1), false, 
                    std::make_shared<Shards>(), mExecutor, []() { return electionTimeoutMs; }, false} {}

    std::shared_ptr<MockStubManager> mMockStub;
    std::shared_ptr<TestExecutor> mExecutor;
    const static auto electionTimeoutMs = 300;

protected:
    RaftNode<MockStubManager> mRaftNode;
};

const int RaftNodeTest::electionTimeoutMs;

TEST_F(RaftNodeTest, LeaderElection) {
    mRaftNode.startElection();

    // Check if the node became a leader
    ASSERT_EQ(mRaftNode.getState().state, RaftState::CANDIDATE);
    ASSERT_EQ(mRaftNode.getState().currentTerm, 1);
}

TEST_F(RaftNodeTest, ReceiveHeartbeat) {
    // Simulate receiving a heartbeat
    mRaftNode.receiveHeartbeat(1, "peerB");

    // Validate state change
    ASSERT_EQ(mRaftNode.getState().state, RaftState::FOLLOWER);
    ASSERT_EQ(mRaftNode.getLeader(), "peerB");
    ASSERT_EQ(mRaftNode.getState().currentTerm, 1);
}

TEST_F(RaftNodeTest, LogReplication) {
    // Mock some log entries
    LogEntry entry1 = {1, "key1", "value1", "PUT"};
    LogEntry entry2 = {1, "key2", "value2", "PUT"};

    mRaftNode.appendToLog(entry1);
    mRaftNode.appendToLog(entry2);

    // Simulate log replication
    mRaftNode.replicateLogToFollowers();

    // run task "replicate_log"
    mExecutor->advanceTime(std::chrono::milliseconds{0});


    // Check replication state
    mRaftNode.tryCommitLogs();

    auto& localMap = mRaftNode.getLocalMap();
    ASSERT_EQ(localMap.get("key1").value(), "value1");
    ASSERT_EQ(localMap.get("key2").value(), "value2");
}

TEST_F(RaftNodeTest, NotYetVotedRecvRequestVoteRPC) {
    RequestVoteRequest request;
    request.set_term(1);
    request.set_candidate_name("peerB");
    request.set_last_log_index(-1);
    request.set_last_log_term(0);

    RequestVoteResponse response;
    mRaftNode.RequestVote(nullptr, &request, &response);

    // Validate vote granting
    ASSERT_TRUE(response.vote_granted());
    ASSERT_EQ(mRaftNode.getState().currentTerm, 1);
    ASSERT_EQ(mRaftNode.getState().votedFor, "peerB");
}

TEST_F(RaftNodeTest, AlreadyVotedRecvRequestVoteRPC) {
    RequestVoteRequest request;
    request.set_term(1);
    request.set_candidate_name("peerA");
    request.set_last_log_index(-1);
    request.set_last_log_term(0);

    RequestVoteResponse response;
    mRaftNode.RequestVote(nullptr, &request, &response);

    // Validate vote granting
    ASSERT_TRUE(response.vote_granted());
    ASSERT_EQ(mRaftNode.getState().currentTerm, 1);
    ASSERT_EQ(mRaftNode.getState().votedFor, "peerA");

    RequestVoteRequest request2;
    request2.set_term(1);
    request2.set_candidate_name("peerB");
    request2.set_last_log_index(-1);
    request2.set_last_log_term(0);

    RequestVoteResponse response2;
    mRaftNode.RequestVote(nullptr, &request2, &response2);

    // Validate no vote granted
    ASSERT_FALSE(response2.vote_granted());
}

TEST_F(RaftNodeTest, AppendEntriesRPC) {
    AppendEntriesRequest request;
    request.set_term(1);
    request.set_leader_name("peerA");
    request.set_commit_index(1);

    LogEntry entry1 = {1, "key1", "value1", "PUT"};
    auto newEntry = request.add_entries();
    newEntry->set_term(entry1.term);
    newEntry->set_key(entry1.key);
    newEntry->set_value(entry1.value);
    newEntry->set_optype(entry1.opType);

    AppendEntriesResponse response;
    mRaftNode.AppendEntries(nullptr, &request, &response);

    // Validate AppendEntries behavior
    ASSERT_TRUE(response.success());
    ASSERT_EQ(mRaftNode.getLeader(), "peerA");
    ASSERT_EQ(mRaftNode.getState().currentTerm, 1);
    ASSERT_EQ(mRaftNode.getState().state, RaftState::FOLLOWER);

    auto& localMap = mRaftNode.getLocalMap();
    ASSERT_EQ(localMap.get("key1").value(), "value1");
}

TEST_F(RaftNodeTest, ElectionCandidatePostTimeout) {

    // Advance time to trigger election start 
    mExecutor->advanceTime(std::chrono::milliseconds(electionTimeoutMs));
    
    ASSERT_TRUE(mRaftNode.getState().state == RaftState::CANDIDATE);
}




class MockStub2 {
public:
    MockStub2(const std::string& peerName,
             std::function<void(const std::string&, const hashmap::RequestVoteRequest&, hashmap::RequestVoteResponse*)> onRequestVote,
             std::function<void(const std::string&, const hashmap::AppendEntriesRequest&, hashmap::AppendEntriesResponse*)> onAppendEntries)
        : mPeerName(peerName), mOnRequestVote(onRequestVote), mOnAppendEntries(onAppendEntries) {}

    grpc::Status RequestVote(grpc::ClientContext* context, const hashmap::RequestVoteRequest& request, hashmap::RequestVoteResponse* response) {
        std::cout << "executing rqeuest vote " << std::endl;
        mOnRequestVote(mPeerName, request, response);
        // Simulate immediate response or set response fields as needed
        return grpc::Status::OK;
    }

    grpc::Status AppendEntries(grpc::ClientContext* context, const hashmap::AppendEntriesRequest& request, hashmap::AppendEntriesResponse* response) {
        mOnAppendEntries(mPeerName, request, response);
        // Simulate immediate response or set response fields as needed
        return grpc::Status::OK;
    }

private:
    std::string mPeerName;
    std::function<void(const std::string&, const hashmap::RequestVoteRequest&, hashmap::RequestVoteResponse*)> mOnRequestVote;
    std::function<void(const std::string&, const hashmap::AppendEntriesRequest&,hashmap::AppendEntriesResponse*)> mOnAppendEntries;
};

class MockStubManager2 {
public:
    std::function<void(const std::string&, const hashmap::RequestVoteRequest&, hashmap::RequestVoteResponse*)> onRequestVote;
    std::function<void(const std::string&, const hashmap::AppendEntriesRequest&, hashmap::AppendEntriesResponse*)> onAppendEntries;

    std::shared_ptr<MockStub2> getStub(const std::string& peer) const {
        return std::make_shared<MockStub2>(peer, onRequestVote, onAppendEntries);
    }
};

// both nodes have same election timeout, hance attempt to vote for each 
TEST(RaftNodeLeaderElectionTest, LeaderElectionDeadlock) {
    // Create executor and timeout generator
    auto executor = std::make_shared<TestExecutor>();
    static const auto electionTimeoutMs = 100;
    auto timeoutGenerator = []() { return electionTimeoutMs; };

    // Create mock stub manager
    auto stubManager = std::make_shared<MockStubManager2>();

    // Create two RaftNodes
    auto localMap1 = std::make_shared<PartitionedHashMap>(1);
    auto shards1 = std::make_shared<Shards>();
    RaftNode<MockStubManager2> node1("node1", "shard1", {"node2"}, *stubManager, localMap1, false, shards1, executor, timeoutGenerator, false);

    auto localMap2 = std::make_shared<PartitionedHashMap>(1);
    auto shards2 = std::make_shared<Shards>();
    RaftNode<MockStubManager2> node2("node2", "shard1", {"node1"}, *stubManager, localMap2, false, shards2, executor, timeoutGenerator, false);

    // Setup RPC callbacks
    stubManager->onRequestVote = [&](const std::string& peerName, const hashmap::RequestVoteRequest& request, hashmap::RequestVoteResponse* response) {
        std::cout << "rqeuesting vote callback" << std::endl; 
        if (peerName == "node1") {
            node1.RequestVote(nullptr, &request, response);
            // You can check or manipulate the response here
        } else if (peerName == "node2") {
            node2.RequestVote(nullptr, &request, response);
        }
        return Status::OK;
    };

    stubManager->onAppendEntries = [&](const std::string& peerName, const hashmap::AppendEntriesRequest& request, hashmap::AppendEntriesResponse* response) {
        std::cout << "appending entry callback" << std::endl;
        if (peerName == "node1") {
            
            node1.AppendEntries(nullptr, &request, response);
        } else if (peerName == "node2") {
            
            node2.AppendEntries(nullptr, &request, response);
        }
        return Status::OK;
    };

    // Advance time to trigger election start 
    executor->advanceTime(std::chrono::milliseconds(electionTimeoutMs));

    // Advance time to trigger voting 
    executor->advanceTime(std::chrono::milliseconds(0));

    // Since both nodes have the same timeout, they both request votes from each other, and neither are granted the respective vote
    // Hence they remain candidates
    EXPECT_EQ(node1.getState().state, RaftState::CANDIDATE);
    EXPECT_EQ(node2.getState().state, RaftState::CANDIDATE);

    
}

// when a leader is elected and sends a heartbeat to its peer, the peer doesn't start an election once it's election-timeout has passed
TEST(RaftNodeLeaderElectionTest, LeaderElectionPeerElectionTimeoutReset)
{
    auto executor = std::make_shared<TestExecutor>();
    auto stubManager = std::make_shared<MockStubManager2>();

    // Create two RaftNodes
    auto localMap1 = std::make_shared<PartitionedHashMap>(1);
    auto shards1 = std::make_shared<Shards>();
    RaftNode<MockStubManager2> node1("node1", "shard1", {"node2"}, *stubManager, localMap1, false, shards1, executor, [](){ return 100;}, false);

    auto localMap2 = std::make_shared<PartitionedHashMap>(1);
    auto shards2 = std::make_shared<Shards>();
    RaftNode<MockStubManager2> node2("node2", "shard1", {"node1"}, *stubManager, localMap2, false, shards2, executor, [](){ return 150;}, false);

    // Setup RPC callbacks
    stubManager->onRequestVote = [&](const std::string& peerName, const hashmap::RequestVoteRequest& request, hashmap::RequestVoteResponse* response) {
        std::cout << "rqeuesting vote callback" << std::endl; 
        if (peerName == "node1") {
            node1.RequestVote(nullptr, &request, response);
            // You can check or manipulate the response here
        } else if (peerName == "node2") {
            node2.RequestVote(nullptr, &request, response);
        }
        return Status::OK;
    };

    stubManager->onAppendEntries = [&](const std::string& peerName, const hashmap::AppendEntriesRequest& request, hashmap::AppendEntriesResponse* response) {
        std::cout << "appending entry callback" << std::endl;
        if (peerName == "node1") {
            
            node1.AppendEntries(nullptr, &request, response);
        } else if (peerName == "node2") {
            node2.AppendEntries(nullptr, &request, response);
        }
        return Status::OK;
    };

    // Advance time to node1's election-timeout 
    executor->advanceTime(std::chrono::milliseconds(100));
    

    // Advance time to trigger voting and heartbeat sending once elected leader
    executor->advanceTime(std::chrono::milliseconds(0));
    executor->advanceTime(std::chrono::milliseconds(0));

    EXPECT_EQ(node1.getState().state, RaftState::LEADER);
    EXPECT_EQ(node2.getState().state, RaftState::FOLLOWER);

    // Advance time to node2's election-timeout - however no election as has received heartbeat recently
    executor->advanceTime(std::chrono::milliseconds(50));

    executor->advanceTime(std::chrono::milliseconds(0));
    executor->advanceTime(std::chrono::milliseconds(0));


    EXPECT_EQ(node1.getState().state, RaftState::LEADER);
    EXPECT_EQ(node2.getState().state, RaftState::FOLLOWER);
}


// preventing node1 sending  heartbeats to node2 once it is leader causes node2 to evntually start an election and become leader
TEST(RaftNodeLeaderElectionTest, HeartbeatNotReceivedStartsElection)
{
    auto executor = std::make_shared<TestExecutor>();    // Create mock stub manager
    auto stubManager = std::make_shared<MockStubManager2>();

    // Create two RaftNodes
    auto localMap1 = std::make_shared<PartitionedHashMap>(1);
    auto shards1 = std::make_shared<Shards>();
    RaftNode<MockStubManager2> node1("node1", "shard1", {"node2"}, *stubManager, localMap1, false, shards1, executor, []() {return 100;}, false);

    auto localMap2 = std::make_shared<PartitionedHashMap>(1);
    auto shards2 = std::make_shared<Shards>();
    RaftNode<MockStubManager2> node2("node2", "shard1", {"node1"}, *stubManager, localMap2, false, shards2, executor, []() {return 150;}, false);

    // Setup RPC callbacks
    stubManager->onRequestVote = [&](const std::string& peerName, const hashmap::RequestVoteRequest& request, hashmap::RequestVoteResponse* response) {
        if (peerName == "node1") {
            node1.RequestVote(nullptr, &request, response);
            // You can check or manipulate the response here
        } else if (peerName == "node2") {
            node2.RequestVote(nullptr, &request, response);
        }
        return Status::OK;
    };



    stubManager->onAppendEntries = [&](const std::string& peerName, const hashmap::AppendEntriesRequest& request, hashmap::AppendEntriesResponse* response) {
        if (peerName == "node1") {
            
            node1.AppendEntries(nullptr, &request, response);
            return Status::OK;
        }
        // node1 doesn't invoke the mock grpc call on node2, hence node2 never receives the hearbeat
        return Status::CANCELLED;
    };

    // Advance time to node1's election-timeout 
    executor->advanceTime(std::chrono::milliseconds(100));
    

    // Advance time to trigger voting and heartbeat sending once elected leader
    executor->advanceTime(std::chrono::milliseconds(0));
    executor->advanceTime(std::chrono::milliseconds(0));
    

    EXPECT_EQ(node1.getState().state, RaftState::LEADER);
    EXPECT_EQ(node2.getState().state, RaftState::FOLLOWER);

    // Advance time to node2's election-timeout - node 2 becomes leader as hasn't received heartbeat from other node1
    executor->advanceTime(std::chrono::milliseconds(50));
    executor->advanceTime(std::chrono::milliseconds(0));


    // roles are now reversed
    EXPECT_EQ(node1.getState().state, RaftState::FOLLOWER);
    EXPECT_EQ(node2.getState().state, RaftState::LEADER);
}







int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}




