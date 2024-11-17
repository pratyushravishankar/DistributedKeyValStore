#pragma once 
#include <unordered_map>
#include <string>

struct ServerConfig {
    std::unordered_map<std::string, std::string> serverAddresses;
};
