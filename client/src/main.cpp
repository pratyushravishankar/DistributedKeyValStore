#include <iostream>
#include <csignal>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/support/channel_arguments.h>
#include "hashmap.grpc.pb.h"
#include "Config.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using hashmap::EraseRequest;
using hashmap::EraseResponse;
using hashmap::GetRequest;
using hashmap::GetResponse;
using hashmap::HashmapService;
using hashmap::PutRequest;
using hashmap::PutResponse;

class HashMapClient
{
public:
    HashMapClient(std::shared_ptr<Channel> channel) : stub_(HashmapService::NewStub(channel)) {}

    bool insert(const std::string &key, const std::string &value)
    {
        PutRequest request;
        auto *kv = request.mutable_kv();
        kv->set_key(key);
        kv->set_value(value);

        PutResponse response;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        Status status = stub_->Put(&context, request, &response);

        // Act upon its status.
        if (status.ok())
        {
            return true;
        }
        else
        {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return false;
        }
        // TODO rmeove bool success flag from PutResponse
    }

    std::optional<std::string> get(const std::string &key)
    {
        GetRequest request;
        request.set_key(key);
        GetResponse response;
        ClientContext context;

        Status status = stub_->Get(&context, request, &response);
        if (status.ok())
        {
            if (response.found())
            {
                return response.value();
            }
            else
            {
                return std::nullopt;
            }
        }
        else
        {
            std::cout << "Error: " << status.error_code() << ": " << status.error_message() << std::endl;
            return std::nullopt;
        }
    }

    bool erase(const std::string &key)
    {

        EraseRequest request;
        request.set_key(key);
        EraseResponse response;
        ClientContext context;

        Status status = stub_->Erase(&context, request, &response);
        return response.success();
    }

private:
    std::unique_ptr<HashmapService::Stub> stub_;
};

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received. Exiting..." << std::endl;
    exit(signum);
}

int main()
{
    // Register signal handler
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler); // Handle Ctrl+C as well

    const auto& config = ClientConfigManager::getInstance();
    grpc::ChannelArguments args;
    args.SetLoadBalancingPolicyName("round_robin");

    auto channel = grpc::CreateCustomChannel(config.serverAddresses, grpc::InsecureChannelCredentials(), args);    
    HashMapClient client{channel};

    std::string command;
    while (true) {
        std::cout << "Enter command (PUT key value, GET key, ERASE key): ";
        std::getline(std::cin, command);

        std::istringstream iss(command);
        std::string action, key, value;
        iss >> action >> key;

        if (action == "PUT") {
            iss >> value;
            bool reply = client.insert(key, value);
            std::cout << "Insert success: " << reply << std::endl;
        } else if (action == "GET") {
            auto maybeValue = client.get(key);
            if (maybeValue) {
                std::cout << "Value found: " << *maybeValue << std::endl;
            } else {
                std::cout << "Value { " << key << " } doesn't exist" << std::endl;
            }
        } else if (action == "ERASE") {
            bool success = client.erase(key);
            std::cout << "Erase success: " << success << std::endl;
        } else {
            std::cout << "Unknown command." << std::endl;
        }

        // Optional: Sleep for a short duration to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}