add_library(wechat_storage STATIC)

file(GLOB_RECURSE STORAGE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(FILTER STORAGE_SOURCES EXCLUDE REGEX ".*/tests/.*")

target_sources(wechat_storage PRIVATE ${STORAGE_SOURCES})

target_include_directories(wechat_storage
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wechat_storage
    PUBLIC
        wechat_core
        wechat_log
)

if(ENABLE_TESTING)
    file(GLOB_RECURSE STORAGE_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(STORAGE_TEST_SOURCES)
        add_executable(test_storage ${STORAGE_TEST_SOURCES})
        target_link_libraries(test_storage PUBLIC wechat_storage GTest::gtest_main)
        gtest_discover_tests(test_storage)
    endif()
endif()
