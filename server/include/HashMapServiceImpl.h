#pragma once

#include "hashmap.pb.h"
#include "hashmap.grpc.pb.h"
#include <grpcpp/grpcpp.h>

#include "StubManager.h"
#include "PartitionedHashMap.h"
#include "ConsistentHashing.h"
#include "Config.h"
#include "RaftNode.h"
#include <ostream>

using grpc::Status;
using grpc::ClientContext;

using hashmap::ForwardPutRequest;
using hashmap::ForwardPutResponse;
using hashmap::ForwardGetRequest;
using hashmap::ForwardGetResponse;
using hashmap::ForwardEraseRequest;
using hashmap::ForwardEraseResponse;


class HashRingManager {
public:
    static const ConsistentHashing<std::hash<std::string>>& getInstance(const ServerConfig& config) {
        static ConsistentHashing<std::hash<std::string>> instance = buildHashRing(config);
        return instance;
    }
private:
    static ConsistentHashing<std::hash<std::string>> buildHashRing(const ServerConfig& config) {
        return ConsistentHashing<std::hash<std::string>>(config.getShards(), /* virtualNodes */ 4);
    }
};




std::ostream& operator<<(std::ostream& os, const ServerInfo& si) {
    return os << "[ " << si.shard << " " << si.name << " ]"; // Added << before si.mServeELr
}


class RealExecutor : public IExecutor {
public:
    void schedule(Task task, std::chrono::milliseconds delay) override {
        std::thread([task, delay]() {
            std::this_thread::sleep_for(delay);
            task.second();
        }).detach();
    }

    std::chrono::milliseconds currentTime() const override {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    }

    void execute(Task task) override {
        std::thread(task.second).detach();
    }
};


int generateRandomTimeout() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(200, 400); // Randomized interval in ms
    return dis(gen);
}



class HashMapServiceImpl : public hashmap::HashmapService::Service
{
public:
// TODO pass in random generator for election timeout
    HashMapServiceImpl(const ServerInfo& serverInfo, size_t numPartitions, const ServerConfig& config) : mShards{std::make_shared<Shards>()}, mLocalMap{std::make_shared<PartitionedHashMap>(numPartitions)}, mRaftNode{serverInfo.name, serverInfo.shard, config.getPeers(serverInfo.name), stubManager, mLocalMap, false, mShards, std::make_shared<RealExecutor>(), []() {
        return generateRandomTimeout();
    }},  mInfo{serverInfo} ,mConfig{config}, mConsistentHashing(HashRingManager::getInstance(config))
    {
    
        // Initialize stubs for other servers (not necssarily in same shard!)
        for (const auto& s : config.servers) {
            if (s.name != serverInfo.name) {
                stubManager.addStub(s.name, s.address);
            }
        }

        // // blindly rewrrite entry for this shard's leader for now
        // for (const auto& s: config.servers) {
        //     mShards.updateLeader(s.shard, s.name);
        // }

        
        // // prevent election from taking place initially for now.
        // if (mShards.getLeader(serverInfo.shard) == serverInfo.name) {
        //     // this node is the leader
        //     mRaftNode.startHeartbeat();
        // }
        
    };

    ::grpc::Status Put(::grpc::ServerContext *context, const ::hashmap::PutRequest *request, ::hashmap::PutResponse *response)
    {
        const auto key = request->kv().key();
        const auto shardName = mConsistentHashing.findShard(key);
        std::cout << "shard to put into " << shardName << std::endl;
        std::cout << "current server info " << mInfo.name << std::endl;
        if (shardName == mInfo.shard && mRaftNode.isLeader()) {
            return put(context, request, response);
        } else {
            const auto serverName = mShards->getLeader(shardName);
            auto stub = stubManager.getStub(serverName);
            ForwardPutRequest forwardRequest;

            copyKV(request->kv(), *(forwardRequest.mutable_kv()));

            ForwardPutResponse forwardResponse;

            ClientContext clientContext;

            std::cout << "fowarding PUT from " << mInfo << " --> " << serverName << std::endl;
            auto res = stub->ForwardedPut(&clientContext, forwardRequest, &forwardResponse);
            response->set_success(forwardResponse.success());
            return res;
        }
    }

    ::grpc::Status Get(::grpc::ServerContext *context, const ::hashmap::GetRequest *request, ::hashmap::GetResponse *response)
    {
        const auto key = request->key();
        const auto shardName = mConsistentHashing.findShard(key);
        if (shardName == mInfo.shard) { // read from any node in the shard
            return get(context, request, response);
        } else {
            const auto serverName = mShards->getLeader(shardName);
            auto stub = stubManager.getStub(serverName);
            ForwardGetRequest forwardRequest;
            forwardRequest.set_key(key);

            ForwardGetResponse forwardResponse;

            ClientContext clientContext;

            std::cout << "fowarding GET from " << mInfo << " --> " << serverName << std::endl;
            auto res =  stub->ForwardedGet(&clientContext, forwardRequest, &forwardResponse);
            response->set_value(forwardResponse.value());
            response->set_found(forwardResponse.found());
            return res;
        }
    }

    ::grpc::Status Erase(::grpc::ServerContext *context, const ::hashmap::EraseRequest *request, ::hashmap::EraseResponse *response)
    {
        const auto key = request->key();
        const auto shardName = mConsistentHashing.findShard(key);
        if (shardName == mInfo.shard && mRaftNode.isLeader()) {
            return erase(context, request, response);
        } else {
            const auto serverName = mShards->getLeader(shardName);
            auto stub = stubManager.getStub(serverName);
            ForwardEraseRequest forwardRequest;
            forwardRequest.set_key(key);

            ForwardEraseResponse forwardResponse;
            
            ClientContext clientContext;

            std::cout << "fowarding ERASE from " << mInfo << " --> " << serverName << std::endl;
            auto res = stub->ForwardedErase(&clientContext, forwardRequest, &forwardResponse);
            response->set_success(forwardResponse.success());
            return res;
        }
    }

 

    ::grpc::Status ForwardedPut(::grpc::ServerContext *context, const ::hashmap::ForwardPutRequest *request, ::hashmap::ForwardPutResponse *response)
    {
        return put(context, request, response);
    }

    ::grpc::Status ForwardedGet(::grpc::ServerContext *context, const ::hashmap::ForwardGetRequest *request, ::hashmap::ForwardGetResponse *response)
    {
        return get(context, request, response);
    }

    ::grpc::Status ForwardedErase(::grpc::ServerContext *context, const ::hashmap::ForwardEraseRequest *request, ::hashmap::ForwardEraseResponse *response)
    {
        return erase(context, request, response);
    }


    template<typename Req_, typename Resp_>
    ::grpc::Status put(::grpc::ServerContext *context, const Req_ *request, Resp_ *response) {

        if (!mRaftNode.isLeader()) {
            return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION, "Not the leader");
        }
        auto& kv = request->kv();
        const auto& k = kv.key();
        const auto& v = kv.value();

        LogEntry entry{
            .term = mRaftNode.getState().currentTerm,
            .key = k,
            .value =  v,
            .opType = "PUT"
        };
        mRaftNode.appendToLog(entry);
        mRaftNode.replicateLogToFollowers();
        
        
        
        // mLocalMap.insert(k, v);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << mInfo << " inserting key: " << k << " value: " << v << std::endl;
        mRaftNode.tryCommitLogs();
        return ::grpc::Status::OK;
    }


    template<typename Req_, typename Resp_>
    ::grpc::Status get(::grpc::ServerContext *context, const Req_ *request, Resp_ *response)
    {
        // if (!mRaftNode.isLeader()) {
        //     return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION, "Not the leader");
        // }

        const auto& key = request->key();
        std::cout << mInfo << " finding key: " << key << std::endl;
        auto maybeValue = mLocalMap->get(key);

        
        if (maybeValue)
        {
            std::cout << "key{ " << key << " } found - value{ " << *maybeValue << " }" << std::endl;
            response->set_value(*maybeValue);
            response->set_found(true);
        }
        else
        {
            std::cout << "key { " << key << "} not found" << std::endl;
            response->set_found(false);
        }
        return ::grpc::Status::OK;
    }

    template<typename Req_, typename Resp_>
    ::grpc::Status erase(::grpc::ServerContext *context, const Req_ *request, Resp_ *response)
    {
        if (!mRaftNode.isLeader()) {
            return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION, "Not the leader");
        }

        const auto& k = request->key();
        LogEntry entry{
            .term = mRaftNode.getState().currentTerm,
            .key = k,
            .value =  "", 
            .opType = "ERASE"
            };
        mRaftNode.appendToLog(entry);
        mRaftNode.replicateLogToFollowers();


        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << mInfo << " erasing key: " << k << std::endl;
        mRaftNode.tryCommitLogs();
        

        // if (success) {
        //     std::cout << mInfo << " sucessfully erased " << k << std::endl;
        // } else {
        //     std::cout << mInfo << " failed to erase " << k << std::endl;
        // }

        response->set_success(true);
        return ::grpc::Status::OK;
    }

    ::grpc::Status AppendEntries(::grpc::ServerContext* context, const hashmap::AppendEntriesRequest* request,
     hashmap::AppendEntriesResponse* response) {
        return mRaftNode.AppendEntries(context, request, response);
    }

    ::grpc::Status RequestVote(::grpc::ServerContext* context, const hashmap::RequestVoteRequest* request, hashmap::RequestVoteResponse* response) {
        return mRaftNode.RequestVote(context, request, response);
    }



private:

    void copyKV(const auto& from, auto& to) {
        to.set_key(from.key());
        to.set_value(from.value());
    }

    ServerInfo mInfo;

    std::shared_ptr<Shards> mShards;
    std::shared_ptr<PartitionedHashMap> mLocalMap;
    const ServerConfig& mConfig;
    ConsistentHashing<std::hash<std::string>> mConsistentHashing;
    ServerStubManager stubManager;
    RaftNode<ServerStubManager> mRaftNode;

    // std::function<void()> m
};