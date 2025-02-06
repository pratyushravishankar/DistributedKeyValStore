#include "HashMapServiceImpl.h"
#include "Config.h"
#include <thread>
#include <vector>
#include <memory>

#include <grpcpp/grpcpp.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

struct ServerWithService
{
    std::unique_ptr<grpc::Server> server;
    std::unique_ptr<HashMapServiceImpl> service;
};

ServerWithService createServer(const ServerInfo &serverInfo, size_t virtualInstances, const ServerConfig &config)
{
    auto service = std::make_unique<HashMapServiceImpl>(serverInfo, virtualInstances, config);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(serverInfo.address, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
    auto server = builder.BuildAndStart();
    return {
        .server = std::move(server),
        .service = std::move(service)};
}

void startServers()
{
    const auto &config = ServerConfigManager::getInstance();
    std::vector<std::thread> serverThreads;

    for (const auto &serverInfo : config.servers)
    {
        serverThreads.emplace_back([&]()
                                   {
            auto server = createServer(serverInfo, 1, config);
            server.server->Wait(); });
    }

    for (auto &thread : serverThreads)
    {
        thread.join();
    }
}

int main()
{
    startServers();
    return 0;
}
