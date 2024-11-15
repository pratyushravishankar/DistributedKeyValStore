find_package(GTest REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../server/include)
add_executable(testConsistentHashing testConsistentHashing.cpp)
target_link_libraries(testConsistentHashing server_lib GTest::GTest GTest::Main)

