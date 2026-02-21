add_library(wechat_core STATIC)

file(GLOB CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB CORE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

target_sources(wechat_core PRIVATE ${CORE_SOURCES} ${CORE_HEADERS})

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
    file(GLOB CORE_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(CORE_TEST_SOURCES)
        add_executable(test_core ${CORE_TEST_SOURCES})
        target_link_libraries(test_core PUBLIC wechat_core GTest::gtest_main)
        gtest_discover_tests(test_core)
    endif()
endif()