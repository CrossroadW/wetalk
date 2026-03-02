# ══════════════════════════════════════════════════
# Executable
# ══════════════════════════════════════════════════

file(GLOB APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB APP_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(wechat_app ${APP_SOURCES} ${APP_HEADERS})

target_compile_features(wechat_app PRIVATE cxx_std_23)

target_include_directories(wechat_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(wechat_app PRIVATE
    QT_NO_KEYWORDS
    PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}"
)

set_target_properties(wechat_app PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    AUTORCC OFF
)

target_link_libraries(wechat_app
    PUBLIC
        wechat_core
        wechat_chat
        wechat_login
        wechat_contacts
        wechat_network
        wechat_log
        Qt6::Widgets
)

target_compile_options(wechat_app
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
            -Wall -Wextra -Wpedantic -Werror=return-type
        >
)

target_compile_definitions(wechat_app
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG _DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<PLATFORM_ID:Windows>:UNICODE _UNICODE NOMINMAX>
)

if(WIN32)
    # Debug: 显示控制台（便于调试），Release: 隐藏控制台
    set_target_properties(wechat_app PROPERTIES
        WIN32_EXECUTABLE $<$<CONFIG:Release>:ON>)
endif()
