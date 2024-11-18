#pragma once

#include <map>
#include <iostream>
#include "HashMap.h"

template<typename HashingFunc>
class ConsistentHashing {
private:
    std::map<size_t, std::string> hashring; // Map of hash to server name
    HashingFunc hashingfunc;

public:
    // Constructor
    ConsistentHashing(const std::vector<std::string>& serverNames, size_t virtualNodes = 1) {
        for (const auto& name : serverNames) {
            addNode(name, virtualNodes);
        }
    }

    // Add a server to the hash ring with virtual nodes
    void addNode(const std::string& serverName, size_t virtualNodes = 1) {
        for (size_t i = 0; i < virtualNodes; ++i) {
            std::string virtualNodeName = serverName + "#" + std::to_string(i);
            size_t hash = hashingfunc(virtualNodeName);
            hashring[hash] = serverName; // Map hash to server name
        }
    }

    // Remove a server from the hash ring
    void removeNode(const std::string& serverName, size_t virtualNodes = 1) {
        for (size_t i = 0; i < virtualNodes; ++i) {
            std::string virtualNodeName = serverName + "#" + std::to_string(i);
            size_t hash = hashingfunc(virtualNodeName);
            hashring.erase(hash);
        }
    }

    // Find the server responsible for a given key
    std::string findServer(const std::string& key) {
        size_t hash = hashingfunc(key);
        auto it = hashring.lower_bound(hash);
        std::string serverName;
        if (it == hashring.end()) {
            serverName = hashring.begin()->second;
        } else {
            serverName =  it->second;
        }
        std::cout << std::endl;
        std::cout << "key{ " << key << " } " << " found on server { " << serverName << " }" << std::endl;
        return serverName;
    }
};
