#pragma once

#include <unordered_map>
#include <string_view>
#include <string>
#include <optional>

class HashMap
{
public:
    std::unordered_map<std::string, std::string> kv_store;

    HashMap(std::string_view name);
    void insert(const std::string &key, const std::string &value);
    std::optional<std::string> get(const std::string &key);
    bool erase(const std::string& key);
    std::string mName;

private:

    // for now, each HashMap shares their data read from disk
    static constexpr std::string_view mFileName = "data/initial_data.txt";
};
