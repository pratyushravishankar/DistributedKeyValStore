#include "PartitionedHashMap.h"

PartitionedHashMap::PartitionedHashMap(std::string_view filename, size_t numPartitions)
    : numPartitions(numPartitions)
{
    for (size_t i = 0; i < numPartitions; ++i)
    {
        partitions.emplace_back(filename); // Initialize each partition
    }
}

size_t PartitionedHashMap::getPartitionIndex(const std::string &key)
{
    std::hash<std::string> hasher;
    return hasher(key) % numPartitions; // Use member variable
}

void PartitionedHashMap::insert(const std::string &key, const std::string &value)
{
    size_t index = getPartitionIndex(key);
    partitions[index].insert(key, value);
}

std::optional<std::string> PartitionedHashMap::get(const std::string &key)
{
    size_t index = getPartitionIndex(key);
    return partitions[index].get(key);
}

bool PartitionedHashMap::erase(const std::string &key)
{
    size_t index = getPartitionIndex(key);
    return partitions[index].erase(key);
}