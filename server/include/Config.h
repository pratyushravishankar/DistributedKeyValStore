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

    auto getPeers(const std::string& serverName) const {
        
        std::string shard;
        auto it = std::ranges::find(servers, serverName, &ServerInfo::name);
        if (it == servers.end()) {
            std::runtime_error("help");    
        } 
        shard = it->shard;
        std::vector<std::string> peers;
        for (const auto& s : servers) {
            if (s.name != serverName && s.shard == shard) {
                peers.push_back(s.name);
            }
        }        
        return peers;
    }
};

struct ServerConfigManager {
    static const ServerConfig& getInstance() {
        static ServerConfig config{
            .servers = {
                {"S1", "0.0.0.0:50051", "SHARD1"},
                {"S2", "0.0.0.0:50052", "SHARD1"},
                {"S3", "0.0.0.0:50053", "SHARD1"}
            }
        };
        return config;
    }
};

