add_executable(client src/main.cpp)
target_link_libraries(client
    PRIVATE
        proto_library
        ${Protobuf_LIBRARIES}
)