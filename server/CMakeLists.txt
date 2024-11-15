include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(server_lib STATIC src/PartitionedHashMap.cpp src/HashMap.cpp)
add_executable(server src/main.cpp src/PartitionedHashMap.cpp src/HashMap.cpp)
target_link_libraries(server
    PRIVATE
    proto_library
)