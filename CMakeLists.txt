
cmake_minimum_required(VERSION 3.29)
project(DistributedKeyValStore)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_BUILD_TYPE Debug)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Add subdirectories
add_subdirectory(proto)
add_subdirectory(server)
add_subdirectory(client)
