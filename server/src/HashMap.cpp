#include <iostream>
#include <fstream>
#include <unordered_map>
#include "HashMap.h"

HashMap::HashMap()
{
    // Load initial data from file
    std::ifstream input_file(mFileName);
    if (input_file.is_open())
    {
        std::string key, value;
        while (input_file >> key >> value)
        {
            kv_store[key] = value;
        }
        input_file.close();
    }
    else
    {
        std::cerr << "Unable to open initial data file" << std::endl;
    }
}

void HashMap::insert(const std::string &key, const std::string &value)
{
    std::lock_guard<std::mutex> lock(mMutex);
    kv_store[key] = value;
}

std::optional<std::string> HashMap::get(const std::string &key)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (auto it = kv_store.find(key); it != kv_store.end())
    {
        return it->second;
    }
    else
    {
        return std::nullopt;
    }
}

bool HashMap::erase(const std::string &key)
{
    std::lock_guard<std::mutex> lock(mMutex);
    return kv_store.erase(key) > 0;
}