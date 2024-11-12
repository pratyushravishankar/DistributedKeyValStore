#include "HashMap.h"
#include <iostream>
#include <unordered_map>
#include "hashmap.pb.h"
#include "hashmap.grpc.pb.h"
#include <grpcpp/grpcpp.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using hashmap::PutRequest;
using hashmap::PutResponse;

class HashMapServiceImpl final : public hashmap::HashmapService::Service
{
public:
    HashMapServiceImpl(const std::string &filename) : map{filename}
    {
        std::cout << "loaded data from " << filename << std::endl;
    };

    ::grpc::Status Put(::grpc::ServerContext *context, const ::hashmap::PutRequest *request, ::hashmap::PutResponse *response)
    {
        auto kv = request->kv();

        auto k = kv.key();
        auto v = kv.value();

        std::cout << "inserting key: " << k << " value: " << v << std::endl;
        map.insert(kv.key(), kv.value());
        return ::grpc::Status::OK;
    }

    ::grpc::Status Get(::grpc::ServerContext *context, const ::hashmap::GetRequest *request, ::hashmap::GetResponse *response)
    {
        auto key = request->key();
        std::cout << "finding key: " << key << std::endl;
        auto maybeValue = map.get(key);
        if (maybeValue)
        {
            response->set_value(*maybeValue);
            response->set_found(true);
        }
        else
        {
            response->set_value("");
            response->set_found(false);
        }
        return ::grpc::Status::OK;
    }

    ::grpc::Status Erase(::grpc::ServerContext *context, const ::hashmap::EraseRequest *request, ::hashmap::EraseResponse *response)
    {
        auto k = request->key();
        std::cout << "erasing key: " << k << std::endl;
        auto success = map.erase(k);
        response->set_success(success);
        return ::grpc::Status::OK;
    }

private:
    HashMap map;
};

void RunServer(uint16_t port)
{
    std::string server_address = "0.0.0.0:" + std::to_string(port);
    HashMapServiceImpl service{"data/initial_data.txt"};
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main()
{

    // HashMap map("data/initial_data.txt");
    // // std::unordered_map<std::string, std::string> map;
    // // Example usage
    // std::cout << "Initial key-value store size: " << map.kv_store.size() << std::endl;

    // for (const auto &[k, v] : map.kv_store)
    // {
    //     std::cout << k << " -> " << v << std::endl;
    // }
    RunServer(50051);
}
