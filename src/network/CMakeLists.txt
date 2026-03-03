# ══════════════════════════════════════════════════
# Library
# ══════════════════════════════════════════════════

add_library(wechat_network STATIC)

file(GLOB NETWORK_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB NETWORK_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB NETWORK_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/wechat/network/*.h)

target_sources(wechat_network
    PRIVATE
        ${NETWORK_SOURCES}
        ${NETWORK_HEADERS}
        ${NETWORK_PUBLIC_HEADERS}
)

target_include_directories(wechat_network
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wechat_network
    PUBLIC
        wechat_core
        wechat_cache
        
        Qt6::Core
        Qt6::WebSockets
        SQLiteCpp
        nlohmann_json::nlohmann_json
        Boost::headers
        Boost::random
)

target_compile_definitions(wechat_network PRIVATE
        QT_NO_KEYWORDS
)

set_target_properties(wechat_network PROPERTIES
        AUTOMOC ON
        AUTOUIC OFF
        AUTORCC OFF
)

# ══════════════════════════════════════════════════
# Tests
# ══════════════════════════════════════════════════

if(ENABLE_TESTING)
    # 单元测试
    file(GLOB NETWORK_UNIT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/*.cpp)
    file(GLOB NETWORK_UNIT_TEST_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/*.h)
    if(NETWORK_UNIT_TEST_SOURCES)
        add_executable(test_network
            ${NETWORK_UNIT_TEST_SOURCES}
            ${NETWORK_UNIT_TEST_HEADERS}
        )
        target_include_directories(test_network PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_network PUBLIC
            wechat_network
            GTest::gtest_main
        )
        gtest_discover_tests(test_network)
    endif()
endif()
