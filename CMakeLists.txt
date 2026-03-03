cmake_minimum_required(VERSION 3.24)

project(MyProject
        VERSION 1.0.0
        DESCRIPTION "WeChat Clone Application"
        LANGUAGES CXX
)

option(ENABLE_TESTING "Enable testing" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif ()

find_package(spdlog CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Core Widgets Network WebSockets Test)
find_package(SQLiteCpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS headers random exception)
find_package(ZXing CONFIG REQUIRED)

if (ENABLE_TESTING)
    include(CTest)
    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)
endif ()

include_directories(include)

# Modules
add_subdirectory(src/core)
add_subdirectory(src/cache)
add_subdirectory(src/network)
add_subdirectory(src/chat)
add_subdirectory(src/login)
add_subdirectory(src/contacts)
add_subdirectory(src/moments)
add_subdirectory(src/app)
#add_subdirectory(src/grpc/)
