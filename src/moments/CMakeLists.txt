add_library(wechat_moments STATIC)

file(GLOB MOMENTS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB MOMENTS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

target_sources(wechat_moments PRIVATE ${MOMENTS_SOURCES} ${MOMENTS_HEADERS})

target_include_directories(wechat_moments
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wechat_moments
    PUBLIC
        wechat_core
        wechat_network
        wechat_storage
        Qt6::Core
        Qt6::Widgets
        wechat_log
)

if(ENABLE_TESTING)
    file(GLOB MOMENTS_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(MOMENTS_TEST_SOURCES)
        add_executable(test_moments ${MOMENTS_TEST_SOURCES})
        target_link_libraries(test_moments PUBLIC wechat_moments GTest::gtest_main)
        gtest_discover_tests(test_moments)
    endif()

    file(GLOB MOMENTS_SANDBOX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.cpp)
    if(MOMENTS_SANDBOX_SOURCES)
        add_executable(sandbox_moments ${MOMENTS_SANDBOX_SOURCES})
        target_link_libraries(sandbox_moments PUBLIC wechat_moments Qt6::Widgets)
    endif()
endif()
