add_library(wechat_auth STATIC)

file(GLOB_RECURSE AUTH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(FILTER AUTH_SOURCES EXCLUDE REGEX ".*/tests/.*")
list(FILTER AUTH_SOURCES EXCLUDE REGEX ".*/sandbox/.*")

target_sources(wechat_auth PRIVATE ${AUTH_SOURCES})

target_include_directories(wechat_auth
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wechat_auth
    PUBLIC
        wechat_core
        wechat_network
        wechat_storage
        wechat_log
)

if(ENABLE_TESTING)
    file(GLOB_RECURSE AUTH_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(AUTH_TEST_SOURCES)
        add_executable(test_auth ${AUTH_TEST_SOURCES})
        target_link_libraries(test_auth PUBLIC wechat_auth GTest::gtest_main)
        gtest_discover_tests(test_auth)
    endif()

    file(GLOB_RECURSE AUTH_SANDBOX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.cpp)
    if(AUTH_SANDBOX_SOURCES)
        add_executable(sandbox_auth ${AUTH_SANDBOX_SOURCES})
        target_link_libraries(sandbox_auth PUBLIC wechat_auth Qt6::Widgets)
    endif()
endif()
