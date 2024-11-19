find_package(GTest REQUIRED)



include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../server/include ${CMAKE_CURRENT_BINARY_DIR}/proto)
add_executable(testConsistentHashing testConsistentHashing.cpp)
target_link_libraries(testConsistentHashing server_lib GTest::GTest GTest::Main)


add_executable(testRaftNode testRaftNode.cpp)
target_link_libraries(testRaftNode server_lib GTest::GTest GTest::Main 
    proto_library)

