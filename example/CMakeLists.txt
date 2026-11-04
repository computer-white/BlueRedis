add_executable(blueSpider
    spider/main.cpp
    spider/fetcher.cpp
    spider/crawler.cpp
)
target_link_libraries(blueSpider PRIVATE blue_static)
target_include_directories(blueSpider PRIVATE ${CMAKE_SOURCE_DIR}/src)