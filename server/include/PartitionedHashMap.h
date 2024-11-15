#pragma once

#include "HashMap.h"
#include <vector>
#include <map>
#include "ConsistentHashing.h"


class PartitionedHashMap
{
public:
    PartitionedHashMap(std::string_view name, size_t numPartitions);
    void insert(const std::string &key, const std::string &value);
    std::optional<std::string> get(const std::string &key);
    bool erase(const std::string &key);

private:
    ConsistentHashing<std::hash<std::string>> ch;
};