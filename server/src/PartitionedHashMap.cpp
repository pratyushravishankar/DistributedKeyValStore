#include "PartitionedHashMap.h"
#include <iostream>
#include <map>
#include <ranges>

PartitionedHashMap::PartitionedHashMap(std::string_view name, size_t numPartitions) : ch{name, numPartitions} {};


void PartitionedHashMap::insert(const std::string &key, const std::string &value)
{
    auto& virtualNode = ch.findNode(key);
    std::cout << "inserting onto virtualNode: " << virtualNode.mName << std::endl;
    virtualNode.insert(key, value);
}

std::optional<std::string> PartitionedHashMap::get(const std::string &key)
{
    auto& virtualNode = ch.findNode(key);
    std::cout << "get called on virtualNode: " << virtualNode.mName << std::endl;
    return virtualNode.get(key);
}

bool PartitionedHashMap::erase(const std::string &key)
{
    auto& virtualNode = ch.findNode(key);
    std::cout << "deleting from virtualNode: " << virtualNode.mName << std::endl;
    return virtualNode.erase(key);
}