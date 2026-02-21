add_library(wechat_chat STATIC)

file(GLOB CHAT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB CHAT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB CHAT_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/wechat/chat/*.h)

target_sources(wechat_chat PRIVATE
    ${CHAT_SOURCES}
    ${CHAT_HEADERS}
    ${CHAT_PUBLIC_HEADERS}
)

target_include_directories(wechat_chat
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 定义项目根路径和 Qt 无关键字模式
target_compile_definitions(wechat_chat PRIVATE
    PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}"
    QT_NO_KEYWORDS
)

set_target_properties(wechat_chat PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    AUTORCC OFF
)

target_link_libraries(wechat_chat
    PUBLIC
        wechat_core
        wechat_network
        wechat_storage
        Qt6::Core
        Qt6::Widgets
        wechat_log
)

if(ENABLE_TESTING)
    file(GLOB CHAT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(CHAT_TEST_SOURCES)
        add_executable(test_chat ${CHAT_TEST_SOURCES})
        target_link_libraries(test_chat PUBLIC wechat_chat
                GTest::gtest_main
                Qt6::Test
        )
        gtest_discover_tests(test_chat)
    endif()

    file(GLOB CHAT_SANDBOX_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.h)
    if(CHAT_SANDBOX_SOURCES)
        qt_add_executable(sandbox_chat ${CHAT_SANDBOX_SOURCES})
        target_compile_definitions(sandbox_chat PRIVATE QT_NO_KEYWORDS)
        set_target_properties(sandbox_chat PROPERTIES AUTOMOC ON)
        target_link_libraries(sandbox_chat PUBLIC wechat_chat Qt6::Widgets)
    endif()
endif()
