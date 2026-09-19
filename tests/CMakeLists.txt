# ==========================================================
# tests/CMakeLists.txt
# ==========================================================

# 只在这里需要 GTest
find_package(GTest REQUIRED)

# 不希望make -j时就跑Google测试
# include(GoogleTest)

# 注册为Google Test,但是在ctest或 make test时才跑测试
macro(add_gtest test_name)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.cpp)
        message(WARNING "tests/${test_name}.cpp 不存在,跳过")
        return()
    endif()

    add_executable(${test_name} ${test_name}.cpp)

    target_link_libraries(${test_name} PRIVATE
        blue_static
        GTest::gtest
        GTest::gtest_main   # 不需要我们手动写main
        GTest::gmock
    )

    # 关键:用 add_test,不用 gtest_discover_tests
    # 这样 make 时不会执行,只有 ctest 时执行
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 60                       # 单测超时 60 秒
        LABELS "unit"                    # 单元测试 使用 ctest -L unit
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    message(STATUS "[gtest] ${test_name} 已添加(仅 ctest 时运行)")
endmacro()

# 只编译,永远不注册为测试
macro(add_blue_exe exe_name)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${exe_name}.cpp)
        message(WARNING "tests/${exe_name}.cpp 不存在,跳过")
        return()
    endif()

    add_executable(${exe_name} ${exe_name}.cpp)
    target_link_libraries(${exe_name} PRIVATE blue_static)

    message(STATUS "[exe]   ${exe_name} 已添加(不会自动运行)")
endmacro()

# Google Test和自带main
macro(add_gtest_custom_main test_name)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.cpp)
        message(WARNING "tests/${test_name}.cpp 不存在,跳过")
        return()
    endif()

    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        blue_static
        GTest::gtest
        GTest::gmock
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 120)

    message(STATUS "[gtest+] ${test_name} 已添加(自定义 main)")
endmacro()


# 单元测试
add_gtest(test_url)
add_gtest(test_task)
add_gtest(test_iomanager)
add_gtest(test_defer)
# add_gtest(test_config)
# add_gtest(test_bytearray)
# add_gtest(test_bytearrayBuffer)
# add_gtest(test_resp_parser)
# add_gtest(test_log_rotate)
# add_gtest(test_redis_aof_config)
# add_gtest(test_proxy_urlrewrite)
# add_gtest(test_debug)

# 服务
add_blue_exe(helloblue)
add_blue_exe(httpserver)
add_blue_exe(blueRedis)
add_blue_exe(httpclient)
add_blue_exe(test_tcpServer)
add_blue_exe(test_echo)
add_blue_exe(test_taskbt)
