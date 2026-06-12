cmake_minimum_required(VERSION 3.14)
project(blue)
set(CMAKE_CXX_COMPILER "g++-12")
set(CMAKE_C_COMPILER "gcc-12")

# 可选：显示详细编译信息
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

# ===== 添加相对路径支持 =====
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # 将源码路径映射为 . (当前目录)
    add_compile_options(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.)
    # 同时处理调试信息中的路径
    add_compile_options(-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.)
    message(STATUS "启用相对路径支持，源码根目录: ${CMAKE_SOURCE_DIR}")
endif()
# ===========================

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启协程支持
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fcoroutines)
endif()
# 错误检测
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
# set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")

# ===== 重点：输出目录设置必须在创建目标之前 =====
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)  # 动态库
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)  # 静态库



# 打印确认（调试用）
message(STATUS "可执行文件输出目录: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "动态库输出目录: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "静态库输出目录: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

# 查找需要的外部库(yaml.cpp, pthread, hiredis)
find_library(YAML_CPP yaml-cpp PATHS /usr/lib/x86_64-linux-gnu)
find_library(PTHREAD_LIB pthread)
find_library(HIREDIS_LIB hiredis)

# 查找 Boost
find_package(Boost REQUIRED COMPONENTS iostreams coroutine context)
# gzip 需要 zlib 底层支持
find_package(ZLIB REQUIRED)
# 查找 json
find_package(nlohmann_json REQUIRED)

# 头文件
include_directories(/usr/include) # yaml-cpp头文件
include_directories(.)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/include)  # llhttp头文件

# 查找外部包
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBIDN2 libidn2)
    pkg_check_modules(GUMBO REQUIRED gumbo)
    if(LIBIDN2_FOUND)
        add_definitions(-DHAVE_LIBIDN2)
        message(STATUS "Found libidn2, IDN support enabled")
    else()
        message(STATUS "libidn2 not found, IDN support disabled")
    endif()

    if(GUMBO_FOUND)
        message(STATUS "Found Gumbo parser: ${GUMBO_INCLUDE_DIRS}")
        include_directories(${GUMBO_INCLUDE_DIRS})
    endif()
endif()

# 查找OpenSSL
find_package(OpenSSL REQUIRED)


# 查找 ragel 工具
find_program(RAGEL_EXECUTABLE ragel)
if(NOT RAGEL_EXECUTABLE)
    message(FATAL_ERROR "ragel not found, please install: sudo apt install ragel")
endif()

# 设置 .rl 文件和输出文件路径
set(RAGEL_INPUT  ${CMAKE_SOURCE_DIR}/blue/url_parser.rl)
set(RAGEL_OUTPUT ${CMAKE_SOURCE_DIR}/blue/urlparser.cpp)

# 自定义命令：.rl → .cpp
add_custom_command(
    OUTPUT ${RAGEL_OUTPUT}
    COMMAND ${RAGEL_EXECUTABLE} -G2 -o ${RAGEL_OUTPUT} ${RAGEL_INPUT}
    DEPENDS ${RAGEL_INPUT}
    COMMENT "Generating URL parser from Ragel: ${RAGEL_INPUT}"
)

# llhttp 源文件
set(LLHTTP_SRC
    ${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/src/llhttp.c
    ${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/src/api.c
    ${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/src/http.c
)

# 设置源文件变量
set(LIB_SRC 
    blue/address.cpp
    blue/async_logger.cpp
    blue/config.cpp
    blue/configinit.cpp
    blue/bytearray.cpp
    blue/dbmanager.cpp
    blue/fdmanager.cpp
    blue/hook.cpp
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
    http/http.cpp
    http/httpconnection.cpp
    http/httpParser.cpp
    http/httpserver.cpp
    http/httpservlet.cpp
    http/httpsession.cpp
    newblue/io_manager.cpp
    newblue/scheduler.cpp
    newblue/timer.cpp
    ${LLHTTP_SRC}
    # 如果有更多源文件，在这里添加
)

macro(check_newblue_file file_name)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/newblue/${file_name}.cpp)
        message(FATAL_ERROR "源文件 newblue/${file_name}.cpp 不存在")
    endif()
endmacro()

check_newblue_file(scheduler)
check_newblue_file(io_manager)
check_newblue_file(timer)

# 检查http源文件是否存在
macro(check_http_file file_name)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/http/${file_name}.cpp)
        message(FATAL_ERROR "源文件 blue/${file_name}.cpp 不存在！")
    endif()
endmacro()

check_http_file(http)
check_http_file(httpconnection)
check_http_file(httpParser)
check_http_file(httpserver)
check_http_file(httpservlet)
check_http_file(httpsession)

# 创建共享库
add_library(blue SHARED ${LIB_SRC})

# 创建静态库
add_library(blue_static STATIC ${LIB_SRC})

# 设置静态库的输出名称
set_target_properties(blue_static PROPERTIES 
    OUTPUT_NAME "blue"  # 生成 libsylar.a
)

# 查找mysql路径
find_path(MYSQL_INCLUDE_DIR
    NAMES mysql.h
    PATHS /usr/include/mysql
)

# 查找mysql库
find_library(MYSQL_CLIENT_LIB
    NAMES mysqlclient
    PATHS /usr/lib/x86_64-linux-gnu
)

if(MYSQL_INCLUDE_DIR AND MYSQL_CLIENT_LIB)
    target_include_directories(blue PRIVATE ${MYSQL_INCLUDE_DIR})
    target_include_directories(blue_static PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(blue PRIVATE ${MYSQL_CLIENT_LIB})
    target_link_libraries(blue_static PRIVATE ${MYSQL_CLIENT_LIB})
    
    set(MYSQL_FOUND TRUE)
    set(MYSQL_LIB ${MYSQL_CLIENT_LIB})

    message(STATUS "✓ MySQL found: ${MYSQL_INCLUDE_DIR} -> ${MYSQL_CLIENT_LIB}")
else()
    set(MYSQL_FOUND FALSE)
    message(WARNING "⚠ MySQL not found")
endif()

# 链接boost,OpenSSL
target_link_libraries(blue PRIVATE
    ${OPENSSL_LIBRARIES}
    Boost::iostreams
    Boost::coroutine
    Boost::context
    ZLIB::ZLIB
    nlohmann_json::nlohmann_json
)
target_link_libraries(blue_static PRIVATE
    ${OPENSSL_LIBRARIES}
    Boost::iostreams
    Boost::coroutine
    Boost::context
    ZLIB::ZLIB
    nlohmann_json::nlohmann_json
)

# 链接 IDN 库（如果找到）
if(LIBIDN2_FOUND)
    target_include_directories(blue PRIVATE ${LIBIDN2_INCLUDE_DIRS})
    target_link_libraries(blue PRIVATE ${LIBIDN2_LIBRARIES})
    
    target_include_directories(blue_static PRIVATE ${LIBIDN2_INCLUDE_DIRS})
    target_link_libraries(blue_static PRIVATE ${LIBIDN2_LIBRARIES})
endif()

# 链接 GUMBO
if(GUMBO_FOUND)
    target_include_directories(blue PRIVATE ${GUMBO_INCLUDE_DIRS})
    target_link_libraries(blue PRIVATE ${GUMBO_LIBRARIES})
    target_include_directories(blue_static PRIVATE ${GUMBO_INCLUDE_DIRS})
    target_link_libraries(blue_static PRIVATE ${GUMBO_LIBRARIES})
endif()

set(LINK_LIBS blue ${YAML_CPP} ${PTHREAD_LIB} ${HIREDIS_LIB})

list(APPEND LINK_LIBS 
    ${OPENSSL_LIBRARIES}
    Boost::iostreams
    Boost::coroutine
    Boost::context
    ZLIB::ZLIB)

# 链接mysql
if (MYSQL_FOUND)
    list(APPEND LINK_LIBS ${MYSQL_LIB})
endif()

# 链接 IDN
if(LIBIDN2_FOUND)
    list(APPEND LINK_LIBS ${LIBIDN2_LIBRARIES})
endif()

# 链接 GUMBO
if(GUMBO_FOUND)
    list(APPEND LINK_LIBS ${GUMBO_LIBRARIES})
endif()

# ========== 创建测试可执行文件 ==========
macro(add_newblue_test test_name)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/newtests/${test_name}.cpp)
        add_executable(${test_name} newtests/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE ${LINK_LIBS})
        message(STATUS "测试程序 ${test_name} 已添加")
    else()
        message(WARNING "newtests/${test_name}.cpp 不存在，跳过测试程序")
    endif()
endmacro()

add_newblue_test(test_new_iomanager)
add_newblue_test(test_echo)
add_newblue_test(test_new_msocket)
add_newblue_test(test_compute)
add_newblue_test(test_tcpServer)
add_newblue_test(test_httpserver)
add_newblue_test(test_httpconnection)
add_newblue_test(test_benchmark)
add_newblue_test(test_debug)
add_newblue_test(test_asynclog)
add_newblue_test(test_resp)
add_newblue_test(test_commandHandler)
add_newblue_test(test_resp_parser)
add_newblue_test(test_redis_stress)
add_newblue_test(test_sharded_redis)

# 打印构建信息
message(STATUS "========================")
message(STATUS "项目: ${PROJECT_NAME}")
message(STATUS "源文件: ${LIB_SRC}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================")
