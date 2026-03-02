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
    # 单元测试 (test_*.cpp, 不包含 screen)
    file(GLOB LOGIN_UNIT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp)
    if(LOGIN_UNIT_TEST_SOURCES)
        add_executable(test_login ${LOGIN_UNIT_TEST_SOURCES})
        target_compile_definitions(test_login PRIVATE QT_NO_KEYWORDS)
        target_link_libraries(test_login PUBLIC wechat_login
                GTest::gtest_main
                Qt6::Test
        )
        gtest_discover_tests(test_login)
    endif()

    # 截图测试 (*_screen_test.cpp)
    file(GLOB LOGIN_SCREEN_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*_screen_test.cpp)
    if(LOGIN_SCREEN_TEST_SOURCES)
        qt_add_executable(test_login_screen ${LOGIN_SCREEN_TEST_SOURCES})
        target_compile_definitions(test_login_screen PRIVATE QT_NO_KEYWORDS
            PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}")
        set_target_properties(test_login_screen PROPERTIES AUTOMOC ON)
        target_link_libraries(test_login_screen PUBLIC wechat_login Qt6::Test Qt6::Widgets)
    endif()

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
