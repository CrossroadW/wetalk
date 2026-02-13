cmake_minimum_required(VERSION 3.24)

project(MyProject
    VERSION 1.0.0
    DESCRIPTION "WeChat Clone Application"
    LANGUAGES CXX
)

option(ENABLE_TESTING "Enable testing" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

find_package(spdlog REQUIRED CONFIG)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

if(ENABLE_TESTING)
    include(CTest)
    find_package(GTest REQUIRED CONFIG)
    include(GoogleTest)
endif()

include_directories(include)

# Modules
add_subdirectory(src/log)
add_subdirectory(src/core)
add_subdirectory(src/storage)
add_subdirectory(src/network)
add_subdirectory(src/auth)
add_subdirectory(src/chat)
add_subdirectory(src/contacts)
add_subdirectory(src/moments)

# Main app
add_executable(myapp)

target_sources(myapp
    PRIVATE
        src/main.cpp
)

target_compile_features(myapp PRIVATE cxx_std_23)

target_link_libraries(myapp
    PUBLIC
        wechat_core
        wechat_auth
        wechat_chat
        wechat_contacts
        wechat_moments
        wechat_log
        Qt6::Widgets
)

target_compile_options(myapp
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
            -Wall -Wextra -Wpedantic -Werror=return-type
        >
)

target_compile_definitions(myapp
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG _DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<PLATFORM_ID:Windows>:UNICODE _UNICODE NOMINMAX>
)

if(WIN32)
    set_target_properties(myapp PROPERTIES WIN32_EXECUTABLE OFF)
endif()

