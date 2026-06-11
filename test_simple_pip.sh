#!/bin/bash

echo "Testing pipeline with simple commands..."
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

# 测试1: 简单管道
echo "=== Test 1: Simple pipeline ==="
printf "PING\r\nSET test hello\r\nGET test\r\n" | nc 127.0.0.1 6666
echo ""

# 测试2: RESP格式的管道
echo "=== Test 2: RESP format pipeline ==="
cat > /tmp/commands.txt << 'EOF'
*1
$4
PING
*3
$3
SET
$4
test
$5
hello
*2
$3
GET
$4
test
EOF

cat /tmp/commands.txt | nc 127.0.0.1 6379
echo ""

# 测试3: 使用 redis-cli --pipe 但使用简单命令
echo "=== Test 3: redis-cli --pipe simple ==="
echo "SET key1 value1" | redis-cli --pipe
echo "GET key1" | redis-cli --pipe
echo "DEL key1" | redis-cli --pipe

# 测试4: 批量 SET
echo "=== Test 4: Batch SET ==="
for i in {1..10}; do
    echo "SET batch_key_$i value_$i"
done | redis-cli --pipe

echo "GET batch_key_5" | redis-cli --pipe

# 清理
kill $SERVER_PID

echo "Pipeline test completed!"