add_library(wechat_chat STATIC)

file(GLOB_RECURSE CHAT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(FILTER CHAT_SOURCES EXCLUDE REGEX ".*/tests/.*")
list(FILTER CHAT_SOURCES EXCLUDE REGEX ".*/sandbox/.*")

target_sources(wechat_chat PRIVATE ${CHAT_SOURCES})

target_include_directories(wechat_chat
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
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
        add_executable(sandbox_chat ${CHAT_SANDBOX_SOURCES})
        target_link_libraries(sandbox_chat PUBLIC wechat_chat Qt6::Widgets)
    endif()
endif()
