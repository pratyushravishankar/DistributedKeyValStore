cmake_minimum_required(VERSION 3.29)
project(DistributedKeyValStore)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(proto)