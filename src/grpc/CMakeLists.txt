# ══════════════════════════════════════════════════
# gRPC Module
# ══════════════════════════════════════════════════

# ══════════════════════════════════════════════════
# Protobuf 生成
# ══════════════════════════════════════════════════

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

message(STATUS "_GRPC_PLUGIN_PATH: ${_GRPC_PLUGIN_PATH}")
# 找到 grpc_cpp_plugin
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)


if (NOT _GRPC_CPP_PLUGIN_EXECUTABLE)
    message(FATAL_ERROR "grpc_cpp_plugin not found")
endif ()

set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)


# Proto file
get_filename_component(AUTH_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/proto/auth.proto" ABSOLUTE)
get_filename_component(AUTH_PROTO_PATH "${AUTH_PROTO}" PATH)

# Generated sources
set(AUTH_PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/auth.pb.cc")
set(AUTH_PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/auth.pb.h")
set(AUTH_GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/auth.grpc.pb.cc")
set(AUTH_GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/auth.grpc.pb.h")

add_custom_command(
        OUTPUT "${AUTH_PROTO_SRCS}" "${AUTH_PROTO_HDRS}" "${AUTH_GRPC_SRCS}" "${AUTH_GRPC_HDRS}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${AUTH_PROTO_PATH}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${AUTH_PROTO}"
        DEPENDS "${AUTH_PROTO}"
        COMMENT "Generating gRPC and protobuf code for auth.proto"
)

# ══════════════════════════════════════════════════
# Library
# ══════════════════════════════════════════════════

add_library(wechat_grpc STATIC)

file(GLOB GRPC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB GRPC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

target_sources(wechat_grpc
        PRIVATE
        ${GRPC_SOURCES}
        ${GRPC_HEADERS}
        ${AUTH_PROTO_SRCS}
        ${AUTH_GRPC_SRCS}
)

target_include_directories(wechat_grpc
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(wechat_grpc
        PUBLIC
        wechat_network  # 依赖网络模块接口
        wechat_core
        wechat_log
        gRPC::grpc++
        protobuf::libprotobuf
)

# ══════════════════════════════════════════════════
# Tests
# ══════════════════════════════════════════════════

if (ENABLE_TESTING)
    file(GLOB GRPC_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if (GRPC_TEST_SOURCES)
        add_executable(test_grpc ${GRPC_TEST_SOURCES})
        target_include_directories(test_grpc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_grpc PUBLIC wechat_grpc GTest::gtest_main)
        gtest_discover_tests(test_grpc)
    endif ()
endif ()