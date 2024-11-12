cmake_minimum_required(VERSION 3.29)
project(ProtoProject)

set(CMAKE_CXX_STANDARD 23)

# if(DEFINED ENV{MY_INSTALL_DIR})
#     set(CMAKE_PREFIX_PATH $ENV{MY_INSTALL_DIR})
# else()
#     message(WARNING "MY_INSTALL_DIR is not set. Using default path.")
#     set(CMAKE_PREFIX_PATH "/usr/local/lib")
# endif()

find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

find_package(absl REQUIRED)

# find_package(Abseil REQUIRED)

# Specify the .proto files
set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/hashmap.proto)

add_library(proto_library ${PROTO_FILES})
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET proto_library LANGUAGE cpp)
protobuf_generate(TARGET proto_library LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

# Include the Protobuf include directories
target_include_directories(proto_library PUBLIC ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(proto_library PUBLIC protobuf::libprotobuf gRPC::grpc++ gRPC::grpc)

message(STATUS "PROTO_SRCS: ${PROTO_SRCS}")
message(STATUS "PROTO_HDRS: ${PROTO_HDRS}")


