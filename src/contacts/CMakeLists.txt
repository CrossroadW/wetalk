add_library(wechat_contacts STATIC)

file(GLOB CONTACTS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB CONTACTS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

target_sources(wechat_contacts PRIVATE ${CONTACTS_SOURCES} ${CONTACTS_HEADERS})

target_include_directories(wechat_contacts
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wechat_contacts
    PUBLIC
        wechat_core
        wechat_network
        wechat_storage
        Qt6::Core
        Qt6::Widgets
        wechat_log
)

if(ENABLE_TESTING)
    file(GLOB CONTACTS_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    if(CONTACTS_TEST_SOURCES)
        add_executable(test_contacts ${CONTACTS_TEST_SOURCES})
        target_link_libraries(test_contacts PUBLIC wechat_contacts GTest::gtest_main)
        gtest_discover_tests(test_contacts)
    endif()

    file(GLOB CONTACTS_SANDBOX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.cpp)
    if(CONTACTS_SANDBOX_SOURCES)
        add_executable(sandbox_contacts ${CONTACTS_SANDBOX_SOURCES})
        target_link_libraries(sandbox_contacts PUBLIC wechat_contacts Qt6::Widgets)
    endif()
endif()
