add_library(wechat_contacts STATIC)

file(GLOB CONTACTS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB CONTACTS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB CONTACTS_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/wechat/contacts/*.h)

target_sources(wechat_contacts PRIVATE
    ${CONTACTS_SOURCES}
    ${CONTACTS_HEADERS}
    ${CONTACTS_PUBLIC_HEADERS}
)

target_include_directories(wechat_contacts
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(wechat_contacts PRIVATE
    QT_NO_KEYWORDS
)

set_target_properties(wechat_contacts PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    AUTORCC OFF
)

target_link_libraries(wechat_contacts
    PUBLIC
        wechat_core
        wechat_network
        Qt6::Core
        Qt6::Widgets
)

if(ENABLE_TESTING)
    # 单元测试
    file(GLOB CONTACTS_UNIT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/*.cpp)
    file(GLOB CONTACTS_UNIT_TEST_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/*.h)
    if(CONTACTS_UNIT_TEST_SOURCES)
        add_executable(test_contacts_presenter
            ${CONTACTS_UNIT_TEST_SOURCES}
            ${CONTACTS_UNIT_TEST_HEADERS}
        )
        target_link_libraries(test_contacts_presenter PUBLIC
            wechat_contacts
            GTest::gtest_main
            Qt6::Test
        )
        gtest_discover_tests(test_contacts_presenter)
    endif()

    # Sandbox
    file(GLOB CONTACTS_SANDBOX_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.h)
    if(CONTACTS_SANDBOX_SOURCES)
        qt_add_executable(sandbox_contacts ${CONTACTS_SANDBOX_SOURCES})
        target_compile_definitions(sandbox_contacts PRIVATE QT_NO_KEYWORDS)
        set_target_properties(sandbox_contacts PROPERTIES AUTOMOC ON)
        target_link_libraries(sandbox_contacts PUBLIC wechat_contacts Qt6::Widgets)
    endif()
endif()
