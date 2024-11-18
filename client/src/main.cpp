#include <iostream>

#include <grpcpp/grpcpp.h>
#include "hashmap.grpc.pb.h"

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

int main()
{

    const std::string target = "localhost:50051";

    HashMapClient client{
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials())};
    auto reply = client.insert("vronsky", "adam");

    std::cout << "insert success " << reply << std::endl;

    client.insert("zebra", "marty");

    auto find = [&](const std::string& key) {
        auto maybeValue = client.get(key);
        if (maybeValue)
        {
            std::cout << "value found : " << *maybeValue << std::endl;
        }
        else
        {
            std::cout << "value { " << key << " } doesn't exist" << std::endl;
        }
    };

    find("cat");
    find("dog");
    find("gorilla");
    find("llama");
    find("monkey");
    find("giraffe");
    find("zebra");





    auto success = client.erase("vronsky");
    client.erase("llama");
    std::cout << "Erasing sucess: " << success << std::endl;
}