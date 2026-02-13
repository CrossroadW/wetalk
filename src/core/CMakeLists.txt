add_library(wechat_core STATIC)

file(GLOB_RECURSE CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*/tests/.*")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*/sandbox/.*")

target_sources(wechat_core PRIVATE ${CORE_SOURCES})

target_include_directories(wechat_core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(wechat_core PUBLIC cxx_std_23)

target_link_libraries(wechat_core
    PUBLIC
        wechat_log
        Boost::headers
        SQLiteCpp
)

# Unit tests
if(ENABLE_TESTING)
    file(GLOB_RECURSE CORE_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(CORE_TEST_SOURCES)
        add_executable(test_core ${CORE_TEST_SOURCES})
        target_link_libraries(test_core PUBLIC wechat_core GTest::gtest_main)
        gtest_discover_tests(test_core)
    endif()
endif()