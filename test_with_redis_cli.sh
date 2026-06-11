#!/bin/bash

# 编译你的服务器
echo "Building server..."
make -j8 && clear

# 启动服务器
echo "Starting server..."
bin/test_commandHandler &
SERVER_PID=$!

# 等待服务器启动
sleep 2

# 测试函数
test_command() {
    echo "Testing: $1"
    echo -e "$2" | redis-cli --pipe
    echo "------------------------"
}

# 基础命令测试
echo "========== Basic Commands Test =========="

# PING
redis-cli PING

# SET 和 GET
redis-cli SET test_key "Hello Redis"
redis-cli GET test_key

# 批量操作
redis-cli MSET key1 value1 key2 value2 key3 value3
redis-cli MGET key1 key2 key3

# 数字操作
redis-cli SET counter 100
redis-cli INCR counter
redis-cli INCRBY counter 50
redis-cli DECR counter

# 列表操作
redis-cli LPUSH mylist "world"
redis-cli LPUSH mylist "hello"
redis-cli LRANGE mylist 0 -1

# 哈希操作
redis-cli HSET user:1000 name "John"
redis-cli HSET user:1000 age "30"
redis-cli HGET user:1000 name
redis-cli HGETALL user:1000

# 集合操作
redis-cli SADD myset "a" "b" "c"
redis-cli SMEMBERS myset

# 过期时间
redis-cli SETEX expiring_key 5 "I will expire"
sleep 6
redis-cli GET expiring_key

echo "========== Pipeline Test =========="
# 管道测试
{
    echo "SET key1 value1"
    echo "SET key2 value2"
    echo "GET key1"
    echo "GET key2"
    echo "DEL key1 key2"
    echo "PING"
} | redis-cli --pipe

echo "========== Concurrent Test =========="
# 并发测试
for i in {1..10}; do
    redis-cli SET con_key_$i "value_$i" &
done
wait

for i in {1..10}; do
    redis-cli GET con_key_$i &
done
wait

echo "========== Large Data Test =========="
# 大数据测试
LARGE_STRING=$(python3 -c "print('x' * 100000)")
redis-cli SET large_key "$LARGE_STRING"
redis-cli GET large_key | wc -c

echo "========== Error Handling Test =========="
# 错误命令
redis-cli INVALID_COMMAND
redis-cli GET

# 清理
echo "Cleaning up..."
redis-cli FLUSHALL

# 停止服务器
kill $SERVER_PID

echo "\n✅ All tests completed!"
