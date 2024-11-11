#include <iostream>
#include <fstream>
#include <unordered_map>
#include "HashMap.h"

HashMap::HashMap(std::string_view filename)
{
    // Load initial data from file
    std::ifstream input_file(filename);
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
    kv_store[key] = value;
}

std::string HashMap::get(const std::string &key)
{
    return kv_store[key];
}
