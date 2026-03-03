add_library(wechat_cache STATIC)

file(GLOB CACHE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB CACHE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

target_sources(wechat_cache PRIVATE
    ${CACHE_SOURCES}
    ${CACHE_HEADERS}
)

target_include_directories(wechat_cache
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(wechat_cache PRIVATE
    QT_NO_KEYWORDS
)

set_target_properties(wechat_cache PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    AUTORCC OFF
)

target_link_libraries(wechat_cache
    PUBLIC
        wechat_core
        
        Qt6::Core
        SQLiteCpp
        nlohmann_json::nlohmann_json
        Boost::headers
        Boost::random
)
