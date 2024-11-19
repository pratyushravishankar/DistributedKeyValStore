#include "StubManager.h"
#include <grpcpp/grpcpp.h>

void ServerStubManager::addStub(const std::string& serverName, const std::string& serverAddress) {
    auto channel = grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials());
    stubs[serverName] = HashmapService::NewStub(channel);
}

std::shared_ptr<HashmapService::Stub> ServerStubManager::getStub(const std::string& serverName) const  {
    return stubs.at(serverName);
}