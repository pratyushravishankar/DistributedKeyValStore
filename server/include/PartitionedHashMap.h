#pragma once

#include "HashMap.h"
#include <vector>



class PartitionedHashMap {
public:
    PartitionedHashMap(size_t numPartitions);

    void insert(const std::string& key, const std::string& value);

    std::optional<std::string> get(const std::string& key);

    bool erase(const std::string& key);

private:

    size_t mNumPartitions;
    std::vector<HashMap> partitions;
    std::hash<std::string> hashingfunc;

    HashMap& getPartition(const std::string& key);
};
