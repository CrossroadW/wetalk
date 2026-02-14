add_library(wechat_chat STATIC)

file(GLOB_RECURSE CHAT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(FILTER CHAT_SOURCES EXCLUDE REGEX ".*/tests/.*")
list(FILTER CHAT_SOURCES EXCLUDE REGEX ".*/sandbox/.*")

# 分别列出头文件，以确保 AUTOMOC 正确处理
set(CHAT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/MessageListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MessageItemWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MessageUtils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatWidget.h
)

target_sources(wechat_chat PRIVATE
    ${CHAT_SOURCES}
    ${CHAT_HEADERS}
)

target_include_directories(wechat_chat
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 定义项目根路径
target_compile_definitions(wechat_chat PRIVATE
    PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}"
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
    file(GLOB_RECURSE CHAT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(CHAT_TEST_SOURCES)
        add_executable(test_chat ${CHAT_TEST_SOURCES})
        target_link_libraries(test_chat PUBLIC wechat_chat GTest::gtest_main)
        gtest_discover_tests(test_chat)
    endif()

    file(GLOB_RECURSE CHAT_SANDBOX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.cpp)
    if(CHAT_SANDBOX_SOURCES)
        qt_add_executable(sandbox_chat ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/main.cpp)
        target_link_libraries(sandbox_chat PUBLIC wechat_chat Qt6::Widgets)
    endif()
endif()
