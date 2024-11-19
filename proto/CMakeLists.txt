cmake_minimum_required(VERSION 3.29)
project(ProtoProject)

set(CMAKE_CXX_STANDARD 23)

# Specify the .proto files
set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/hashmap.proto)

# Use system-installed protoc and grpc_cpp_plugin
find_program(_PROTOBUF_PROTOC protoc REQUIRED)
find_program(_GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# Generate Protobuf and gRPC files
set(GENERATED_FILES_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(
    OUTPUT ${GENERATED_FILES_DIR}/hashmap.pb.cc ${GENERATED_FILES_DIR}/hashmap.pb.h
    COMMAND ${_PROTOBUF_PROTOC} --cpp_out=${GENERATED_FILES_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
)

add_custom_command(
    OUTPUT ${GENERATED_FILES_DIR}/hashmap.grpc.pb.cc ${GENERATED_FILES_DIR}/hashmap.grpc.pb.h
    COMMAND ${_PROTOBUF_PROTOC} --grpc_out=${GENERATED_FILES_DIR} --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN} -I ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
)

# Add the generated files to proto_library
add_library(proto_library STATIC
    ${GENERATED_FILES_DIR}/hashmap.pb.cc
    ${GENERATED_FILES_DIR}/hashmap.grpc.pb.cc
)

# Include generated headers
target_include_directories(proto_library PUBLIC ${GENERATED_FILES_DIR})

# Link against gRPC and Protobuf targets
target_link_libraries(proto_library PUBLIC protobuf::libprotobuf gRPC::grpc++)
