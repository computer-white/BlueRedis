cmake_minimum_required(VERSION 3.14)
project(blue)

# ============ 编译器 ============
# set(CMAKE_CXX_COMPILER "g++-12")
# set(CMAKE_C_COMPILER   "gcc-12")
if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(GXX13 g++-13)
    if(GXX13)
        set(CMAKE_CXX_COMPILER "g++-13")
        set(CMAKE_C_COMPILER   "gcc-13")
    endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============ 全局编译选项 ============
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.)
    add_compile_options(-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.)
    message(STATUS "启用相对路径支持,源码根目录: ${CMAKE_SOURCE_DIR}")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fcoroutines>)
endif()

# 编译时使用cmake .. -DENABLE_ASAN=ON
option(ENABLE_ASAN  "Address Sanitizer"              OFF)
option(ENABLE_UBSAN "Undefined Behavior Sanitizer"   OFF)

if(ENABLE_ASAN)
    message("ASAN ON")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN)
    message("UBSAN ON")
    add_compile_options(-fsanitize=undefined -fno-sanitize-recover=all -g)
    add_link_options(-fsanitize=undefined)
endif()

# add_compile_options(-Wall -Wextra -Wpedantic)

# ============ 输出目录 ============
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# ============ 外部依赖 ============
find_library(YAML_CPP     yaml-cpp PATHS /usr/lib/x86_64-linux-gnu)
find_library(PTHREAD_LIB  pthread)
find_library(HIREDIS_LIB  hiredis)

find_package(Boost REQUIRED COMPONENTS iostreams coroutine context)
find_package(ZLIB REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(absl REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig)

# ============ IDN / Gumbo ============
set(BLUE_HAS_IDN    OFF)
set(BLUE_HAS_GUMBO  OFF)

if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBIDN2 libidn2)
    pkg_check_modules(GUMBO REQUIRED gumbo)

    if(LIBIDN2_FOUND)
        add_definitions(-DHAVE_LIBIDN2)
        set(BLUE_HAS_IDN ON)
        message(STATUS "Found libidn2, IDN support enabled")
    else()
        message(STATUS "libidn2 not found, IDN support disabled")
    endif()

    if(GUMBO_FOUND)
        set(BLUE_HAS_GUMBO ON)
        message(STATUS "Found Gumbo parser: ${GUMBO_INCLUDE_DIRS}")
    endif()
endif()

# ============ ragel ============
find_program(RAGEL_EXECUTABLE ragel)
if(NOT RAGEL_EXECUTABLE)
    message(FATAL_ERROR "ragel not found, please install: sudo apt install ragel")
endif()

set(RAGEL_INPUT  ${CMAKE_SOURCE_DIR}/blue/url_parser.rl)
set(RAGEL_OUTPUT ${CMAKE_SOURCE_DIR}/blue/urlparser.cpp)

add_custom_command(
    OUTPUT ${RAGEL_OUTPUT}
    COMMAND ${RAGEL_EXECUTABLE} -G2 -o ${RAGEL_OUTPUT} ${RAGEL_INPUT}
    DEPENDS ${RAGEL_INPUT}
    COMMENT "Generating URL parser from Ragel: ${RAGEL_INPUT}"
)

# ============ 头文件目录 ============
include_directories(/usr/include)
include_directories(.)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/include)
if(GUMBO_FOUND)
    include_directories(${GUMBO_INCLUDE_DIRS})
endif()

# ============ llhttp 源码 ============
set(LLHTTP_SRC
    ${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/src/llhttp.c
    ${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/src/api.c
    ${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/src/http.c
)

# ============ 库源文件 ============
set(LIB_SRC
    blue/address.cpp
    blue/config.cpp
    blue/configinit.cpp
    blue/bytearray.cpp
    blue/dbmanager.cpp
    blue/fdmanager.cpp
    blue/log.cpp
    blue/msocket.cpp
    blue/msocketstream.cpp
    blue/mstream.cpp
    blue/mthread.cpp
    blue/mysqlpool.cpp
    blue/redismanager.cpp
    blue/sslsocket.cpp
    blue/tcpServer.cpp
    blue/url.cpp
    blue/urlparser.cpp
    blue/util.cpp
    blue/scheduler.cpp
    blue/io_manager.cpp
    blue/timer.cpp
    blue/resp_parser.cpp
    blue/config_parser.cpp
    redis_command/modules/subscription.cpp
    redis_command/modules/slowlog.cpp
    redis_command/modules/monitor.cpp
    redis_command/modules/AOF.cpp
    redis_command/modules/replication.cpp
    http/http.cpp
    http/httpconnection.cpp
    http/httpParser.cpp
    http/httpserver.cpp
    http/httpservlet.cpp
    http/httpsession.cpp
    newblue/io_manager.cpp
    newblue/scheduler.cpp
    newblue/timer.cpp
    proxy/url_rewriter.cpp
    ${LLHTTP_SRC}
)

# 生成 urlparser.cpp 必须在编译 blue 之前
set_source_files_properties(blue/urlparser.cpp PROPERTIES
    GENERATED TRUE)

# 让 urlparser.cpp 依赖 custom command
add_custom_target(urlparser_gen DEPENDS ${RAGEL_OUTPUT})
# 把生成文件加入源列表(注意:先加入再让库依赖 target)
list(APPEND LIB_SRC ${RAGEL_OUTPUT})

# ============ MySQL ============
find_path(MYSQL_INCLUDE_DIR NAMES mysql.h PATHS /usr/include/mysql)
find_library(MYSQL_CLIENT_LIB NAMES mysqlclient PATHS /usr/lib/x86_64-linux-gnu)

# ============ 库目标 ============
add_library(blue        SHARED ${LIB_SRC})
add_library(blue_static STATIC ${LIB_SRC})

set_target_properties(blue_static PROPERTIES OUTPUT_NAME "blue")

add_dependencies(blue        urlparser_gen)
add_dependencies(blue_static urlparser_gen)

# 公共 include(测试通过 link blue 就能拿到)
target_include_directories(blue PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/include
)
target_include_directories(blue_static PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/include
)

# 公共依赖(用 PUBLIC,这样下游测试不用重复写)
set(BLUE_PUBLIC_LIBS
    ${YAML_CPP}
    ${PTHREAD_LIB}
    ${HIREDIS_LIB}
    ${OPENSSL_LIBRARIES}
    Boost::iostreams
    Boost::coroutine
    Boost::context
    ZLIB::ZLIB
    nlohmann_json::nlohmann_json
    absl::flat_hash_map
    absl::hash
)

target_link_libraries(blue        PUBLIC  ${BLUE_PUBLIC_LIBS})
target_link_libraries(blue_static PUBLIC  ${BLUE_PUBLIC_LIBS})

if(MYSQL_INCLUDE_DIR AND MYSQL_CLIENT_LIB)
    target_include_directories(blue        PRIVATE ${MYSQL_INCLUDE_DIR})
    target_include_directories(blue_static PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(blue        PRIVATE ${MYSQL_CLIENT_LIB})
    target_link_libraries(blue_static PRIVATE ${MYSQL_CLIENT_LIB})
    message(STATUS "✓ MySQL found: ${MYSQL_INCLUDE_DIR} -> ${MYSQL_CLIENT_LIB}")
else()
    message(WARNING "⚠ MySQL not found")
endif()

if(BLUE_HAS_IDN)
    target_include_directories(blue        PRIVATE ${LIBIDN2_INCLUDE_DIRS})
    target_link_libraries(blue             PRIVATE ${LIBIDN2_LIBRARIES})
    target_include_directories(blue_static PRIVATE ${LIBIDN2_INCLUDE_DIRS})
    target_link_libraries(blue_static      PRIVATE ${LIBIDN2_LIBRARIES})
endif()

if(BLUE_HAS_GUMBO)
    target_include_directories(blue        PRIVATE ${GUMBO_INCLUDE_DIRS})
    target_link_libraries(blue             PRIVATE ${GUMBO_LIBRARIES})
    target_include_directories(blue_static PRIVATE ${GUMBO_INCLUDE_DIRS})
    target_link_libraries(blue_static      PRIVATE ${GUMBO_LIBRARIES})
endif()

# ============ 子目录 ============
# 只有开了 BUILD_TESTING(默认 ON)才进 tests
option(BLUE_BUILD_TESTS "Build tests" ON)
if(BLUE_BUILD_TESTS)
    enable_testing()                # 顶层调用一次即可
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()

# 打印
message(STATUS "========================")
message(STATUS "项目: ${PROJECT_NAME}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================")