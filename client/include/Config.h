#pragma once 

#include <vector>
#include <string>

struct ClientConfig {
    const std::string serverAddresses;
};
 
struct ClientConfigManager {
    static const ClientConfig& getInstance() {
        static ClientConfig config{.serverAddresses = 
        
            "ipv4:127.0.0.1:50051,127.0.0.1:50052,127.0.0.1:50053"
        };
        return config;
    }
};

