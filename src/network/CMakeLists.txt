add_library(wechat_network STATIC)

file(GLOB_RECURSE NETWORK_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(FILTER NETWORK_SOURCES EXCLUDE REGEX ".*/tests/.*")

target_sources(wechat_network PRIVATE ${NETWORK_SOURCES})

target_include_directories(wechat_network
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wechat_network
    PUBLIC
        wechat_core
        wechat_log
)

if(ENABLE_TESTING)
    file(GLOB_RECURSE NETWORK_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(NETWORK_TEST_SOURCES)
        add_executable(test_network ${NETWORK_TEST_SOURCES})
        target_include_directories(test_network PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_network PUBLIC wechat_network GTest::gtest_main)
        gtest_discover_tests(test_network)
    endif()
endif()
