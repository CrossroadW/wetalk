add_library(wechat_log STATIC)

file(GLOB LOG_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB LOG_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

target_sources(wechat_log PRIVATE ${LOG_SOURCES} ${LOG_HEADERS})

target_include_directories(wechat_log
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(wechat_log PUBLIC cxx_std_23)

target_link_libraries(wechat_log
    PUBLIC
        spdlog::spdlog
)

# Sandbox
file(GLOB LOG_SANDBOX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.cpp)
if(LOG_SANDBOX_SOURCES)
    add_executable(sandbox_log ${LOG_SANDBOX_SOURCES})
    target_link_libraries(sandbox_log PUBLIC wechat_log)
endif()
