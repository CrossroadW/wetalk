add_library(wechat_storage STATIC)

file(GLOB STORAGE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB STORAGE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB CHAT_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/wechat/storage/*.h)

target_sources(wechat_storage PRIVATE ${STORAGE_SOURCES} ${STORAGE_HEADERS} ${CHAT_PUBLIC_HEADERS})

target_include_directories(wechat_storage
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wechat_storage
    PUBLIC
        wechat_core
        wechat_log
        SQLiteCpp
        nlohmann_json::nlohmann_json
)

if(ENABLE_TESTING)
    file(GLOB STORAGE_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(STORAGE_TEST_SOURCES)
        add_executable(test_storage ${STORAGE_TEST_SOURCES})
        target_link_libraries(test_storage PUBLIC wechat_storage GTest::gtest_main)
        gtest_discover_tests(test_storage)
    endif()
endif()
