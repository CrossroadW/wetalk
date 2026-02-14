# ══════════════════════════════════════════════════
# Protobuf 生成
# ══════════════════════════════════════════════════

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# 找到 grpc_cpp_plugin
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

if(NOT _GRPC_CPP_PLUGIN_EXECUTABLE)
    # 尝试从 CONAN 缓存路径找
    file(GLOB GRPC_DIRS "C:/Users/31093/.conan2/p/b/grpc*")
    foreach(GRPC_DIR ${GRPC_DIRS})
        find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin
            HINTS "${GRPC_DIR}/p/bin"
            NO_DEFAULT_PATH)
        if(_GRPC_CPP_PLUGIN_EXECUTABLE)
            break()
        endif()
    endforeach()
endif()

if(NOT _GRPC_CPP_PLUGIN_EXECUTABLE)
    message(FATAL_ERROR "grpc_cpp_plugin not found")
endif()

message(STATUS "Found grpc_cpp_plugin: ${_GRPC_CPP_PLUGIN_EXECUTABLE}")

if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

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

add_library(wechat_network STATIC)

file(GLOB_RECURSE NETWORK_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(FILTER NETWORK_SOURCES EXCLUDE REGEX ".*/tests/.*")

target_sources(wechat_network
    PRIVATE
        ${NETWORK_SOURCES}
        ${AUTH_PROTO_SRCS}
        ${AUTH_GRPC_SRCS}
)

target_include_directories(wechat_network
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wechat_network
    PUBLIC
        wechat_core
        wechat_log
        gRPC::grpc++
        protobuf::libprotobuf
)

# ══════════════════════════════════════════════════
# Tests
# ══════════════════════════════════════════════════

if(ENABLE_TESTING)
    file(GLOB_RECURSE NETWORK_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(NETWORK_TEST_SOURCES)
        add_executable(test_network ${NETWORK_TEST_SOURCES})
        target_include_directories(test_network PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_network PUBLIC wechat_network GTest::gtest_main)
        gtest_discover_tests(test_network)
    endif()
endif()
