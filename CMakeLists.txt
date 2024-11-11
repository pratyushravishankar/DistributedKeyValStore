cmake_minimum_required(VERSION 3.29)
project(DistributedKeyValStore)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(proto)



# include_directories(include)

# set(SOURCE_DIR src)




# if(NOT DEFINED _GRPC_CPP_PLUGIN_EXECUTABLE)
# message(FATAL_ERROR "gRPC plugin not found. Please ensure gRPC is installed and accessible.")
# endif()

# # Proto file
# get_filename_component(hm_proto "proto/hashmap.proto" ABSOLUTE)
# get_filename_component(hm_proto_path "${hm_proto}" PATH)

# set(hm_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/hashmap.pb.cc")
# set(hm_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/hashmap.pb.h")
# set(hm_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/hashmap.grpc.pb.cc")
# set(hm_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/hashmap.grpc.pb.h")
# add_custom_command(
#       OUTPUT "${hm_proto_srcs}" "${hm_proto_hdrs}" "${hm_grpc_srcs}" "${hm_grpc_hdrs}"
#       COMMAND ${_PROTOBUF_PROTOC}
#       ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
#         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
#         -I "${hm_proto_path}"
#         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
#         "${hm_proto}"
#       DEPENDS "${hm_proto}"
#       COMMENT "Generating protobuf and gRPC code from ${hm_proto}"
# )

# include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# # hm_grpc_proto
# add_library(hm_grpc_proto
#   ${hm_grpc_srcs}
#   ${hm_grpc_hdrs}
#   ${hm_proto_srcs}
#   ${hm_proto_hdrs})
# target_link_libraries(hm_grpc_proto
#   absl::check
#   ${_REFLECTION}
#   ${_GRPC_GRPCPP}
#   ${_PROTOBUF_LIBPROTOBUF})

# add_executable(KeyValStore
#     src/main.cpp
#     src/HashMap.cpp
#     ${SOURCE_DIR}
# )

# target_link_libraries(KeyValStore
#     hm_grpc_proto
#     gRPC::grpc++
#     protobuf::libprotobuf
#     protobuf::libprotoc
# )