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



# Specify the .proto files
set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/hashmap.proto)

# Generate C++ source and header files from .proto
# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
# grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_FILES})

# # Compile protobuf and grpc files in myproto target to cpp
# #


add_library(proto_library ${PROTO_FILES})
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET proto_library LANGUAGE cpp)
protobuf_generate(TARGET proto_library LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")



# Create a static library for the generated code (optional)
# add_library(proto_library ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS})


# Include the Protobuf include directories
target_include_directories(proto_library PUBLIC ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(proto_library PUBLIC protobuf::libprotobuf gRPC::grpc++ gRPC::grpc)

message(STATUS "PROTO_SRCS: ${PROTO_SRCS}")
message(STATUS "PROTO_HDRS: ${PROTO_HDRS}")


