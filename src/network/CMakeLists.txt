# ══════════════════════════════════════════════════
# Library
# ══════════════════════════════════════════════════

add_library(wechat_network STATIC)

file(GLOB NETWORK_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB NETWORK_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

target_sources(wechat_network
    PRIVATE
        ${NETWORK_SOURCES}
        ${NETWORK_HEADERS}
)

target_include_directories(wechat_network
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wechat_network
    PUBLIC
        wechat_core
        wechat_log
)

# ══════════════════════════════════════════════════
# Tests
# ══════════════════════════════════════════════════

if(ENABLE_TESTING)
    file(GLOB NETWORK_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(NETWORK_TEST_SOURCES)
        add_executable(test_network ${NETWORK_TEST_SOURCES})
        target_include_directories(test_network PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_network PUBLIC wechat_network GTest::gtest_main)
        gtest_discover_tests(test_network)
    endif()
endif()
