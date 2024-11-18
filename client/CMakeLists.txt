include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(client src/main.cpp)
target_link_libraries(client
    PRIVATE
        proto_library
)