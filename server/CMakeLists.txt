add_library(server_lib STATIC src/PartitionedHashMap.cpp src/HashMap.cpp src/RaftNode.cpp src/StubManager.cpp)
target_include_directories(server_lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/proto
)
target_link_libraries(server_lib
    PRIVATE
    proto_library
)


add_executable(server src/main.cpp src/PartitionedHashMap.cpp src/HashMap.cpp src/RaftNode.cpp src/StubManager.cpp)
target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/proto
)
target_link_libraries(server
    PRIVATE
    proto_library
)