#!/bin/bash
# test_with_redis_cli.sh

SERVER_BIN="./bin/test_commandHandler"
SERVER_PID=""

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_info() {
    echo -e "${YELLOW}→${NC} $1"
}

# 启动服务器
start_server() {
    print_info "Starting server..."
    $SERVER_BIN > server.log 2>&1 &
    SERVER_PID=$!
    sleep 2
    
    if kill -0 $SERVER_PID 2>/dev/null; then
        print_success "Server started with PID: $SERVER_PID"
        return 0
    else
        print_error "Failed to start server"
        cat server.log
        return 1
    fi
}

# 停止服务器
stop_server() {
    if [ ! -z "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
        print_info "Server stopped"
    fi
}

# 测试命令
test_command() {
    local cmd="$1"
    local expected="$2"
    
    print_info "Testing: $cmd"
    
    # 使用 timeout 防止挂起
    result=$(timeout 5 redis-cli -h 127.0.0.1 -p 6666 $cmd 2>&1)
    local exit_code=$?
    
    if [ $exit_code -eq 124 ]; then
        print_error "Command timeout (5s): $cmd"
        return 1
    elif [ $exit_code -ne 0 ]; then
        print_error "Command failed: $cmd (exit: $exit_code)"
        echo "Output: $result"
        return 1
    fi
    
    if [ ! -z "$expected" ] && [ "$result" != "$expected" ]; then
        print_error "Unexpected result: expected '$expected', got '$result'"
        return 1
    fi
    
    print_success "$cmd -> $result"
    return 0
}

# 管道测试
test_pipeline() {
    print_info "Testing pipeline..."
    
    local commands="SET pipe1 val1\nSET pipe2 val2\nGET pipe1\nGET pipe2\nDEL pipe1 pipe2\n"
    
    result=$(echo -e "$commands" | timeout 5 redis-cli --pipe 2>&1)
    local exit_code=$?
    
    if [ $exit_code -eq 124 ]; then
        print_error "Pipeline timeout"
        return 1
    elif [ $exit_code -ne 0 ]; then
        print_error "Pipeline failed"
        echo "$result"
        return 1
    fi
    
    print_success "Pipeline test passed"
    return 0
}

# 大字符串测试
test_large_string() {
    print_info "Testing large string (10KB)..."
    
    # 生成10KB字符串
    large_string=$(python3 -c "print('x' * 10240)" 2>/dev/null || printf '%0.sx' {1..10240})
    
    # SET
    result=$(echo "SET large_key \"$large_string\"" | timeout 5 redis-cli --pipe 2>&1)
    if [ $? -ne 0 ]; then
        print_error "Failed to SET large string"
        return 1
    fi
    
    # GET 并比较长度
    length=$(redis-cli STRLEN large_key 2>/dev/null)
    if [ "$length" != "10240" ]; then
        print_error "Large string length mismatch: expected 10240, got $length"
        return 1
    fi
    
    print_success "Large string test passed (10240 bytes)"
    return 0
}

# 并发测试
test_concurrent() {
    print_info "Testing concurrent connections (10 clients)..."
    
    # 创建临时脚本
    cat > /tmp/redis_test.sh << 'EOF'
#!/bin/bash
for i in {1..100}; do
    redis-cli SET "concurrent_$$_$i" "value" > /dev/null 2>&1
    redis-cli GET "concurrent_$$_$i" > /dev/null 2>&1
    redis-cli DEL "concurrent_$$_$i" > /dev/null 2>&1
done
echo "OK"
EOF
    
    chmod +x /tmp/redis_test.sh
    
    # 启动10个并发客户端
    pids=()
    for i in {1..10}; do
        timeout 10 /tmp/redis_test.sh &
        pids+=($!)
    done
    
    # 等待所有完成
    failed=0
    for pid in "${pids[@]}"; do
        wait $pid
        if [ $? -ne 0 ]; then
            failed=$((failed + 1))
        fi
    done
    
    rm /tmp/redis_test.sh
    
    if [ $failed -eq 0 ]; then
        print_success "Concurrent test passed"
        return 0
    else
        print_error "Concurrent test had $failed failures"
        return 1
    fi
}

# 内存监控测试
test_memory_leak() {
    print_info "Testing memory leak (1000 commands)..."
    
    local initial_mem=$(ps -o rss= -p $SERVER_PID | tr -d ' ')
    
    for i in {1..1000}; do
        redis-cli SET "leak_$i" "value_$i" > /dev/null 2>&1
        redis-cli GET "leak_$i" > /dev/null 2>&1
        redis-cli DEL "leak_$i" > /dev/null 2>&1
        
        if [ $((i % 200)) -eq 0 ]; then
            local current_mem=$(ps -o rss= -p $SERVER_PID | tr -d ' ')
            print_info "Memory after $i commands: ${current_mem}KB"
        fi
    done
    
    local final_mem=$(ps -o rss= -p $SERVER_PID | tr -d ' ')
    local diff=$((final_mem - initial_mem))
    
    print_info "Memory: ${initial_mem}KB -> ${final_mem}KB (diff: ${diff}KB)"
    
    if [ $diff -gt 1024 ]; then
        print_error "Possible memory leak: +${diff}KB after 1000 commands"
        return 1
    else
        print_success "Memory stable: +${diff}KB"
        return 0
    fi
}

# 主测试流程
main() {
    echo "========================================="
    echo "     Redis Protocol Test Suite"
    echo "========================================="
    echo ""
    
    # 检查 redis-cli
    if ! command -v redis-cli &> /dev/null; then
        print_error "redis-cli not found. Please install redis-tools"
        exit 1
    fi
    
    # 启动服务器
    if ! start_server; then
        exit 1
    fi
    
    # 运行测试
    tests=(
        "test_command 'PING' 'PONG'"
        "test_command 'SET test hello' 'OK'"
        "test_command 'GET test' 'hello'"
        "test_command 'DEL test' '1'"
        # "test_command 'INCR counter' '1'"
        # "test_command 'INCRBY counter 10' '11'"
        "test_command 'LPUSH mylist a' '1'"
        "test_command 'LPUSH mylist b' '2'"
        "test_command 'LRANGE mylist 0 -1' 'b,a'"
        "test_command 'HSET user name alice' '1'"
        "test_command 'HGET user name' 'alice'"
        "test_pipeline"
        "test_large_string"
        "test_concurrent"
        "test_memory_leak"
    )
    
    failed=0
    for test in "${tests[@]}"; do
        echo ""
        if ! eval $test; then
            failed=$((failed + 1))
            # 如果测试失败，检查服务器是否还活着
            if ! kill -0 $SERVER_PID 2>/dev/null; then
                print_error "Server crashed!"
                cat server.log | tail -20
                break
            fi
        fi
        sleep 0.5
    done
    
    echo ""
    echo "========================================="
    if [ $failed -eq 0 ]; then
        print_success "All tests passed! 🎉"
    else
        print_error "Failed $failed tests"
        echo ""
        echo "Last 20 lines of server log:"
        cat server.log | tail -20
    fi
    echo "========================================="
    
    # 清理
    stop_server
}

# 捕获退出信号
trap stop_server EXIT

# 运行主函数
main