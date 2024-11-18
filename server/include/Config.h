#pragma once 
#include <unordered_map>
#include <string>

struct ServerConfig {
    const std::unordered_map<std::string, std::string> serverAddresses;
};


struct ServerConfigManager {
    static const ServerConfig& getInstance() {
        static ServerConfig config{.serverAddresses = {
            {"S1", "0.0.0.0:50051"},
            {"S2", "0.0.0.0:50052"},
            {"S3", "0.0.0.0:50053"}
        }};
        return config;
    }
};

