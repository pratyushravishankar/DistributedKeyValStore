#include "hashmap.pb.h"
#include "hashmap.grpc.pb.h"
#include "StubManager.h"


#include "Config.h"
using grpc::Status;
using grpc::ClientContext;
using hashmap::ForwardEraseRequest;
using hashmap::ForwardGetRequest;
using hashmap::ForwardPutRequest;
using hashmap::ForwardPutResponse;
using hashmap::ForwardGetResponse;
using hashmap::ForwardEraseResponse;

class HashMapServiceImpl : public hashmap::HashmapService::Service
{
public:
    HashMapServiceImpl(ServerConfig config, std::string_view name, size_t numPartitions) : mName{name}, map{name, numPartitions}
    {
        // Initialize stubs for other servers
        for (const auto& [serverName, serverAddress] : config.serverAddresses) {
            if (serverName != name) {
                stubManager.addStub(serverName, serverAddress);
            }
        }
    };

    ::grpc::Status Put(::grpc::ServerContext *context, const ::hashmap::PutRequest *request, ::hashmap::PutResponse *response)
    {
        if (keyHashedHere(request->kv().key())) {
            return put(context, request, response);
        } else {
            auto stub = stubManager.getStub("S3");


            ForwardPutRequest forwardRequest;

            auto* kv = forwardRequest.mutable_kv();
            auto& kvPut = request->kv();
            kv->set_key(kvPut.key());
            kv->set_value(kvPut.value());
            

            ForwardPutResponse forwardResponse;

            ClientContext clientContext;

            std::cout << "fowarding PUT from " << mName << " --> " << "S3" << std::endl;
            auto res = stub->ForwardedPut(&clientContext, forwardRequest, &forwardResponse);
            response->set_success(forwardResponse.success());
            return res;
        }
    }

    ::grpc::Status Get(::grpc::ServerContext *context, const ::hashmap::GetRequest *request, ::hashmap::GetResponse *response)
    {
       if (keyHashedHere(request->key())) {
            return get(context, request, response);
        } else {
            auto stub = stubManager.getStub("S3");
            ForwardGetRequest forwardRequest;
            forwardRequest.set_key(request->key());

            ForwardGetResponse forwardResponse;

            ClientContext clientContext;

            std::cout << "fowarding GET from " << mName << " --> " << "S3" << std::endl;
            auto res =  stub->ForwardedGet(&clientContext, forwardRequest, &forwardResponse);
            response->set_value(forwardResponse.value());
            response->set_found(forwardResponse.found());
            return res;
        }
    }

    ::grpc::Status Erase(::grpc::ServerContext *context, const ::hashmap::EraseRequest *request, ::hashmap::EraseResponse *response)
    {
       if (keyHashedHere(request->key())) {
            return erase(context, request, response);
        } else {
            auto stub = stubManager.getStub("S3");
            ForwardEraseRequest forwardRequest;
            forwardRequest.set_key(request->key());

            ForwardEraseResponse forwardResponse;
            
            ClientContext clientContext;

            std::cout << "fowarding ERASE from " << mName << " --> " << "S3" << std::endl;
            auto res = stub->ForwardedErase(&clientContext, forwardRequest, &forwardResponse);
            response->set_success(forwardResponse.success());
            return res;
        }
    }

 

    ::grpc::Status ForwardedPut(::grpc::ServerContext *context, const ::hashmap::ForwardPutRequest *request, ::hashmap::ForwardPutResponse *response)
    {
        return put(context, request, response);
    }

    ::grpc::Status ForwardedGet(::grpc::ServerContext *context, const ::hashmap::ForwardGetRequest *request, ::hashmap::ForwardGetResponse *response)
    {
        return get(context, request, response);
    }

    ::grpc::Status ForwardedErase(::grpc::ServerContext *context, const ::hashmap::ForwardEraseRequest *request, ::hashmap::ForwardEraseResponse *response)
    {
        return erase(context, request, response);
    }


    template<typename Req_, typename Resp_>
    ::grpc::Status put(::grpc::ServerContext *context, const Req_ *request, Resp_ *response) {
        auto& kv = request->kv();

        auto k = kv.key();
        auto v = kv.value();

        std::cout << "{ " << mName << " } " << "inserting key: " << k << " value: " << v << std::endl;
        map.insert(kv.key(), kv.value());
        return ::grpc::Status::OK;
    }


    template<typename Req_, typename Resp_>
    ::grpc::Status get(::grpc::ServerContext *context, const Req_ *request, Resp_ *response)
    {
        auto key = request->key();
        std::cout << "{ " << mName << " } " << "finding key: " << key << std::endl;
        auto maybeValue = map.get(key);
        if (maybeValue)
        {
            std::cout << "key{ " << key << " } found - value{ " << *maybeValue << " }" << std::endl;
            response->set_value(*maybeValue);
            response->set_found(true);
        }
        else
        {
            std::cout << "key { " << key << "} not found" << std::endl;
            response->set_value("");
            response->set_found(false);
        }
        return ::grpc::Status::OK;
    }

    template<typename Req_, typename Resp_>
    ::grpc::Status erase(::grpc::ServerContext *context, const Req_ *request, Resp_ *response)
    {
        auto k = request->key();
        std::cout << "{ " << mName << " } " << "erasing key: " << k << std::endl;
        auto success = map.erase(k);
        response->set_success(success);
        return ::grpc::Status::OK;
    }

    bool keyHashedHere(std::string_view) {
        return false;
    }

private:
    PartitionedHashMap map;
    std::string_view mName;
    ServerStubManager stubManager;
};