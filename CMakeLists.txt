cmake_minimum_required(VERSION 3.29)
project(DistributedKeyValStore)

set(CMAKE_CXX_STANDARD 23)




include_directories(include)


set(SOURCE_DIR, src)


add_executable(KeyValStore
    src/main.cpp
    src/HashMap.cpp
    ${SOURCE_DIR}
)