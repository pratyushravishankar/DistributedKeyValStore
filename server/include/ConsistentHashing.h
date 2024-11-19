#pragma once

#include <map>
#include <unordered_set>
#include <iostream>
#include "HashMap.h"

template<typename HashingFunc>
class ConsistentHashing {
private:
    std::map<size_t, std::string> hashring; // Map of hash to server name
    HashingFunc hashingfunc;

public:
    // Constructor
    ConsistentHashing(const std::unordered_set<std::string>& serverNames, size_t virtualNodes = 1) {
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

    // Find the shard responsible for a given key
    std::string findShard(const std::string& key) {
        size_t hash = hashingfunc(key);
        auto it = hashring.lower_bound(hash);
        std::string shardName;
        if (it == hashring.end()) {
            shardName = hashring.begin()->second;
        } else {
            shardName =  it->second;
        }
        std::cout << std::endl;
        std::cout << "key{ " << key << " } " << " found on shard { " << shardName << " }" << std::endl;
        return shardName;
    }
};
