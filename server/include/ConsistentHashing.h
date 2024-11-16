#pragma once

#include <map>
#include <iostream>
#include "HashMap.h"

template<typename HashingFunc>
class ConsistentHashing {
private:
    int mNumPartitions;
    std::map<size_t, HashMap> hashring;
    HashingFunc hashingfunc;

    void rebalance(const HashMap& mapFrom, HashMap& mapTo) const {
        // Rebalance the key-value pairs from mapFrom to mapTo
        for (auto& [k, v] : mapFrom.kv_store) {
            mapTo.insert(k, v);
        }
    }

public:
    // Constructor
    ConsistentHashing(std::string_view namePrefix, size_t numPartitions)
        : mNumPartitions(numPartitions) {
        for (size_t i = 0; i < numPartitions; i++) {
            addNode(std::string(namePrefix) + std::to_string(i));
        }
    }

    // testing purposes
    ConsistentHashing(std::vector<HashMap>&& virtualNodes) {
        for (auto&& v : virtualNodes) {
            hashring.emplace(hashingfunc(v.mName), std::move(v));
        }
    }

    // Add a node to the hash ring
    void addNode(const std::string& name) {
        auto hash = hashingfunc(name); // Use the injected hash function
        std::cout << "constructing " << name << std::endl;
        hashring.emplace(hash, HashMap(name));
    }

    // Remove a node from the hash ring
    void removeNode(const std::string& name) {
        auto hash = hashingfunc(name);
        auto it = hashring.find(hash);
        if (it == hashring.end()) {
            return; // node doesn't exist!
        }
        auto& mapFrom = it->second;
        auto nextIt = std::next(it);
        auto& mapTo = (nextIt == hashring.end()) ? hashring.begin()->second : nextIt->second;
        rebalance(mapFrom, mapTo);
        hashring.erase(it);
    }

    // Find the node corresponding to a given key
HashMap& findNode(const std::string& key) {
    auto hash = hashingfunc(key);
    auto it = hashring.lower_bound(hash);
    if (it == hashring.end()) {
        return hashring.begin()->second;
    } else {
        return it->second;
    }
}

};
