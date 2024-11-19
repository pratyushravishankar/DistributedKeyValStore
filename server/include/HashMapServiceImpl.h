#include "hashmap.pb.h"
#include "hashmap.grpc.pb.h"
#include "StubManager.h"
#include "PartitionedHashMap.h"
#include "ConsistentHashing.h"


#include "Config.h"
#include <ostream>
using grpc::Status;
using grpc::ClientContext;
using hashmap::ForwardEraseRequest;
using hashmap::ForwardGetRequest;
using hashmap::ForwardPutRequest;
using hashmap::ForwardPutResponse;
using hashmap::ForwardGetResponse;
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

class Shards {
public:
    std::string getLeader(const std::string& shardName) {
        return shardToLeader.at(shardName);
    }
    
    void updateLeader(const std::string& shardName, const std::string& serverName) {
        shardToLeader[shardName] = serverName;
    }

private:
    std::map<std::string, std::string> shardToLeader;
};



std::ostream& operator<<(std::ostream& os, const ServerInfo& si) {
    return os << "[ " << si.shard << " " << si.name << " ]"; // Added << before si.mServer
}   



class HashMapServiceImpl : public hashmap::HashmapService::Service
{
public:
    HashMapServiceImpl(const ServerInfo& serverInfo, size_t numPartitions, const ServerConfig& config) : mInfo{serverInfo} ,mConfig{config}, mLocalMap{numPartitions}, mConsistentHashing(HashRingManager::getInstance(config)) 
    {
        // Initialize stubs for other servers
        for (const auto& s : config.servers) {
            if (s.name != serverInfo.name) {
                stubManager.addStub(s.name, s.address);
            }
        }

        for (const auto& s: config.servers) {
            mShards.updateLeader(s.shard, s.name);
        }
    };

    ::grpc::Status Put(::grpc::ServerContext *context, const ::hashmap::PutRequest *request, ::hashmap::PutResponse *response)
    {
        const auto key = request->kv().key();
        const auto shardName = mConsistentHashing.findShard(key);
        if (shardName == mInfo.shard) {
            return put(context, request, response);
        } else {
            const auto serverName = mShards.getLeader(shardName);
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
        if (shardName == mInfo.shard) {
            return get(context, request, response);
        } else {
            const auto serverName = mShards.getLeader(shardName);
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
        if (shardName == mInfo.shard) {
            return erase(context, request, response);
        } else {
            const auto serverName = mShards.getLeader(shardName);
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
        auto& kv = request->kv();
        const auto& k = kv.key();
        const auto& v = kv.value();

        std::cout << mInfo << " inserting key: " << k << " value: " << v << std::endl;
        mLocalMap.insert(kv.key(), kv.value());
        return ::grpc::Status::OK;
    }


    template<typename Req_, typename Resp_>
    ::grpc::Status get(::grpc::ServerContext *context, const Req_ *request, Resp_ *response)
    {
        const auto& key = request->key();
        std::cout << mInfo << " finding key: " << key << std::endl;
        auto maybeValue = mLocalMap.get(key);
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
        const auto& k = request->key();
        std::cout << mInfo << " erasing key: " << k << std::endl;
        const auto success = mLocalMap.erase(k);

        if (success) {
            std::cout << mInfo << " sucessfully erased " << k << std::endl;
        } else {
            std::cout << mInfo << " failed to erase " << k << std::endl;
        }

        response->set_success(success);
        return ::grpc::Status::OK;
    }



private:

    void copyKV(const auto& from, auto& to) {
        to.set_key(from.key());
        to.set_value(from.value());
    }

    ServerInfo mInfo;
    Shards mShards;
    PartitionedHashMap mLocalMap;
    const ServerConfig& mConfig;
    ConsistentHashing<std::hash<std::string>> mConsistentHashing;
    ServerStubManager stubManager;
};