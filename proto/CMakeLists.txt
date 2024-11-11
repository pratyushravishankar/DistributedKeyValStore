cmake_minimum_required(VERSION 3.29)
project(ProtoProject)

set(CMAKE_CXX_STANDARD 23)

find_package(Protobuf REQUIRED)

# Specify the .proto files
set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/hashmap.proto)

# Generate C++ source and header files from .proto
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

message(STATUS "PROTO_SRCS: ${PROTO_SRCS}")
message(STATUS "PROTO_HDRS: ${PROTO_HDRS}")


# Create a static library for the generated code (optional)
add_library(proto_library STATIC ${PROTO_SRCS} ${PROTO_HDRS})

# Include the Protobuf include directories
target_include_directories(proto_library PUBLIC ${Protobuf_INCLUDE_DIRS})
