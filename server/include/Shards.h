#pragma once

#include <string>

class Shards {
public:
    std::string getLeader(const std::string& shardName) {
        return shardToLeader.at(shardName);
    }
    
    void updateLeader(const std::string& shardName, const std::string& serverName) {
        shardToLeader[shardName] = serverName;
    }

private:
    std::map<std::string, std::string> shardToLeader;
};