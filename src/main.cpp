#include "HashMap.h"
#include <iostream>
#include <unordered_map>

#include <grpcpp/grpcpp.h>

int main()
{

    HashMap map("data/initial_data.txt");
    // std::unordered_map<std::string, std::string> map;
    // Example usage
    std::cout << "Initial key-value store size: " << map.kv_store.size() << std::endl;

    for (const auto &[k, v] : map.kv_store)
    {
        std::cout << k << " -> " << v << std::endl;
    }
}
