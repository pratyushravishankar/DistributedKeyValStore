#include "HashMap.h"
#include <vector>
#include <map>
#include <iostream>

class ConsistentHashing {
    private:

    int mNumPartitions;

    std::map<size_t, HashMap> hashring;

    void rebalance(const HashMap& mapFrom, HashMap& mapTo) {
        // as these are references, does mapTo take these references and construct the strings directly within the map?
        for (auto&[k, v] : mapFrom.kv_store) {
            mapTo.insert(k, v);
        }
    }

public:
    ConsistentHashing(std::string_view namePrefix, size_t numPartitions) {
        for (size_t i = 0; i < numPartitions; i++) {
            addNode(std::string(namePrefix) + "#" + std::to_string(i));
        }
    }

    void addNode(std::string_view name) {
        auto hash = std::hash<std::string_view>()(name);
        std::cout << "constructing " << name << std::endl;
        hashring.emplace(hash, HashMap(name));
    }

    void removeNode(std::string_view name) {
        auto hash = std::hash<std::string_view>()(name);
        auto it = hashring.find(hash);
        if (it == hashring.end()) {
            return; // node doesn't exist!
        }
        auto& mapFrom = it->second;
        auto& mapTo = (it++ == hashring.end()) ? hashring.begin()->second : it->second;
        rebalance(mapFrom, mapTo);
        hashring.erase(it);
    }

    HashMap& findNode(const std::string& key) {

        auto hash = std::hash<std::string>()(key);
        auto it = hashring.lower_bound(hash);
        if (it == hashring.end()) {
            return hashring.begin()->second;
        } else {
            return it->second;
        }
    }
};

class PartitionedHashMap
{
public:
    PartitionedHashMap(std::string_view name, size_t numPartitions);
    void insert(const std::string &key, const std::string &value);
    std::optional<std::string> get(const std::string &key);
    bool erase(const std::string &key);

private:
    ConsistentHashing ch;
};