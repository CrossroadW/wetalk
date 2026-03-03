add_library(wechat_core STATIC)

file(GLOB CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB CORE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB CORE_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/wechat/core/*.h)

target_sources(wechat_core PRIVATE ${CORE_SOURCES} ${CORE_HEADERS} ${CORE_PUBLIC_HEADERS}
        )

target_include_directories(wechat_core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(wechat_core PUBLIC cxx_std_23)


target_link_libraries(wechat_core
    PUBLIC
        spdlog::spdlog
        SQLiteCpp
        Boost::headers
        Boost::exception
)


if(ENABLE_TESTING)
    # 单元测试
    file(GLOB CORE_UNIT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/*.cpp)
    file(GLOB CORE_UNIT_TEST_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/*.h)
    if(CORE_UNIT_TEST_SOURCES)
        add_executable(test_core
            ${CORE_UNIT_TEST_SOURCES}
            ${CORE_UNIT_TEST_HEADERS}
        )
        target_link_libraries(test_core PUBLIC
            wechat_core
            GTest::gtest_main
        )
        gtest_discover_tests(test_core)
    endif()

    # Tools
    file(GLOB CORE_TOOL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp)
    foreach(TOOL_SOURCE ${CORE_TOOL_SOURCES})
        get_filename_component(TOOL_NAME ${TOOL_SOURCE} NAME_WE)
        add_executable(${TOOL_NAME} ${TOOL_SOURCE})
        target_compile_definitions(${TOOL_NAME} PRIVATE
            PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}")
        target_link_libraries(${TOOL_NAME} PUBLIC wechat_core)

    endforeach()
endif()
