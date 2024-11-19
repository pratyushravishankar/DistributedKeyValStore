#pragma once 
#include <string>
#include <unordered_set>

struct ServerInfo {
    std::string name;
    std::string address;
    std::string shard;
};

struct ServerConfig {
    const std::vector<ServerInfo> servers;

    auto getShards() const {
        std::unordered_set<std::string> shards;
        for (const auto& server : servers) {
            shards.emplace(server.shard);
        }
        return shards;
    }
};

struct ServerConfigManager {
    static const ServerConfig& getInstance() {
        static ServerConfig config{
            .servers = {
                {"S1", "0.0.0.0:50051", "SHARD1"},
                {"S2", "0.0.0.0:50052", "SHARD2"},
                {"S3", "0.0.0.0:50053", "SHARD3"}
            }
        };
        return config;
    }
};

