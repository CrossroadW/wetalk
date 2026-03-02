add_library(wechat_login STATIC)

file(GLOB LOGIN_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB LOGIN_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB LOGIN_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/wechat/login/*.h)

target_sources(wechat_login PRIVATE
    ${LOGIN_SOURCES}
    ${LOGIN_HEADERS}
    ${LOGIN_PUBLIC_HEADERS}
)

target_include_directories(wechat_login
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(wechat_login PRIVATE
    QT_NO_KEYWORDS
)

set_target_properties(wechat_login PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    AUTORCC OFF
)

target_link_libraries(wechat_login
    PUBLIC
        wechat_core
        wechat_network
        Qt6::Core
        Qt6::Widgets
        ZXing::ZXing
)

if(ENABLE_TESTING)
    # 单元测试 (unit/test_*.cpp)
    # 所有 gtest 测试共享一个 exe 和自定义 main
    file(GLOB LOGIN_UNIT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_*.cpp)
    file(GLOB LOGIN_UNIT_TEST_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/*.h)

    if(LOGIN_UNIT_TEST_SOURCES)
        add_executable(test_login
            ${LOGIN_UNIT_TEST_SOURCES}
            ${LOGIN_UNIT_TEST_HEADERS}
        )
        target_compile_definitions(test_login PRIVATE QT_NO_KEYWORDS)
        target_link_libraries(test_login PUBLIC
            wechat_login
            GTest::gtest
            Qt6::Test
            Qt6::Network
        )
        gtest_discover_tests(test_login)
    endif()

    # 截图测试 (screen/*_test.cpp)
    # 每个 .cpp 文件生成一个独立的 exe
    file(GLOB LOGIN_SCREEN_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/screen/*_test.cpp)
    foreach(SCREEN_TEST_FILE ${LOGIN_SCREEN_TEST_SOURCES})
        get_filename_component(TEST_NAME ${SCREEN_TEST_FILE} NAME_WE)

        qt_add_executable(${TEST_NAME} ${SCREEN_TEST_FILE})
        target_compile_definitions(${TEST_NAME} PRIVATE
            QT_NO_KEYWORDS
            PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}"
        )
        target_include_directories(${TEST_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}
        )
        set_target_properties(${TEST_NAME} PROPERTIES AUTOMOC ON)
        target_link_libraries(${TEST_NAME} PUBLIC
            wechat_login
            Qt6::Test
            Qt6::Widgets
        )
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()

    # Sandbox
    file(GLOB LOGIN_SANDBOX_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.h)
    if(LOGIN_SANDBOX_SOURCES)
        qt_add_executable(sandbox_login ${LOGIN_SANDBOX_SOURCES})
        target_compile_definitions(sandbox_login PRIVATE QT_NO_KEYWORDS
            PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}")
        set_target_properties(sandbox_login PROPERTIES AUTOMOC ON)
        target_link_libraries(sandbox_login PUBLIC wechat_login Qt6::Widgets)
    endif()
endif()
