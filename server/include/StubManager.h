#pragma once

#include <grpcpp/grpcpp.h>
#include <unordered_map>
#include <memory>
#include <string>
using hashmap::HashmapService;

class ServerStubManager {
public:
    void addStub(const std::string& serverName, const std::string& serverAddress) {
        auto channel = grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials());
        stubs[serverName] = HashmapService::NewStub(channel);
    }

    std::shared_ptr<HashmapService::Stub> getStub(const std::string& serverName) {
        return stubs.at(serverName);
    }

private:
    std::unordered_map<std::string, std::shared_ptr<HashmapService::Stub>> stubs;
};