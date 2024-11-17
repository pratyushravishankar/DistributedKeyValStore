#include "PartitionedHashMap.h"
#include "HashMapServiceImpl.h"
#include "Config.h"
#include "StubManager.h"
#include <iostream>
#include <unordered_map>
#include <grpcpp/grpcpp.h>
#include <thread>
#include <vector>
#include <memory>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;


struct ServerWithService {
    std::unique_ptr<grpc::Server> server;
    std::unique_ptr<HashMapServiceImpl> service;
};


ServerWithService createServer(uint16_t port, 
                                           const std::string& name, 
                                           size_t virtualInstances, 
                                           const ServerConfig& config) 
{
    std::string server_address = "0.0.0.0:" + std::to_string(port);
    auto service = std::make_unique<HashMapServiceImpl>(config, name, virtualInstances);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());

    auto server = builder.BuildAndStart();
    return ServerWithService(std::move(server), std::move(service));
}



void startServers() {
    ServerConfig config;
    config.serverAddresses = {
        {"S1", "0.0.0.0:50051"},
        {"S2", "0.0.0.0:50052"},
        {"S3", "0.0.0.0:50053"}
    };

    std::vector<std::thread> serverThreads;

    serverThreads.emplace_back([&]() {
        auto [server, service] = createServer(50051, "S1", 1, config);
        server->Wait();
    });

    serverThreads.emplace_back([&]() {
        auto [server, service] = createServer(50052, "S2", 4, config);
        server->Wait();
    });

    serverThreads.emplace_back([&]() {
        auto [server, service] = createServer(50053, "S3", 1, config);
        server->Wait();
    });

    for (auto& thread : serverThreads) {
        thread.join();
    }
}


int main()
{
    startServers();
    return 0;
}
