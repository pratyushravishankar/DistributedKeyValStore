#pragma once


#include <unordered_map>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "hashmap.pb.h"
#include "hashmap.grpc.pb.h"

using hashmap::HashmapService;

class ServerStubManager {
public:
    void addStub(const std::string& serverName, const std::string& serverAddress);

    std::shared_ptr<HashmapService::Stub> getStub(const std::string& serverName) const;

private:
    std::unordered_map<std::string, std::shared_ptr<HashmapService::Stub>> stubs;
};