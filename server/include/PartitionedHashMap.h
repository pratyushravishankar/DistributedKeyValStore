#include "HashMap.h"
#include <vector>

class PartitionedHashMap
{
public:
    PartitionedHashMap(std::string_view filename, size_t numPartitions);
    void insert(const std::string &key, const std::string &value);
    std::optional<std::string> get(const std::string &key);
    bool erase(const std::string &key);

private:
    std::vector<HashMap> partitions;
    size_t numPartitions;

    size_t getPartitionIndex(const std::string &key);
};