#include "PartitionedHashMap.h"
#include <iostream>
#include <map>
#include <ranges>


PartitionedHashMap::PartitionedHashMap(size_t numPartitions) : mNumPartitions(numPartitions), partitions(numPartitions) {}

void PartitionedHashMap::insert(const std::string& key, const std::string& value) {
        auto& partition = getPartition(key);
        partition.insert(key, value);
}
std::optional<std::string> PartitionedHashMap::get(const std::string& key) {
    auto& partition = getPartition(key);
    return partition.get(key);
}

bool PartitionedHashMap::erase(const std::string& key) {
    auto& partition = getPartition(key);
    return partition.erase(key);
}

HashMap& PartitionedHashMap::getPartition(const std::string& key) {
        size_t hash = hashingfunc(key);
        size_t index = hash % mNumPartitions;
    return partitions[index];
}
