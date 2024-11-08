cmake_minimum_required(VERSION 3.29)
project(DistributedKeyValStore)

set(CMAKE_CXX_STANDARD 23)


include_directories(include)

set(SOURCE_DIR, src)


if(DEFINED ENV{MY_INSTALL_DIR})
    set(CMAKE_PREFIX_PATH $ENV{MY_INSTALL_DIR})
else()
    message(WARNING "MY_INSTALL_DIR is not set. Using default path.")
    set(CMAKE_PREFIX_PATH "/usr/local/lib")
endif()

find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")


add_executable(KeyValStore
    src/main.cpp
    src/HashMap.cpp
    ${SOURCE_DIR}
)

target_link_libraries(KeyValStore
    gRPC::grpc++
    protobuf::libprotobuf
    protobuf::libprotoc
)