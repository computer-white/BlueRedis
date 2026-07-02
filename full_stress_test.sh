#!/bin/bash
# 保存为 full_stress_test.sh

cd ~/c_projects/newblue/bin

echo "=========================================="
echo "    完整压力测试 - 最终性能报告"
echo "=========================================="
echo ""

# 启动服务器
./test_commandHandler &
SERVER_PID=$!
sleep 3

# 测试函数
run_test() {
    echo "--- $1 ---"
    redis-benchmark -h 127.0.0.1 -p 6666 -a client123 \
        -c $2 -n $3 -t $4 -q -P $5 2>/dev/null | grep -v "WARNING"
    echo ""
}

# 1. 基础性能
echo "========== 1. 基础性能 =========="
run_test "基础 SET" 50 50000 "set" 1
run_test "基础 GET" 50 50000 "get" 1

# 2. Pipeline 测试
echo "========== 2. Pipeline 测试 =========="
for P in 1 2 4 8 16 32; do
    run_test "Pipeline $P - SET" 50 50000 "set" $P
    run_test "Pipeline $P - GET" 50 50000 "get" $P
done

# 3. 并发测试
echo "========== 3. 并发测试 =========="
for C in 10 50 100 200 500; do
    run_test "并发 $C - SET" $C 100000 "set" 8
    run_test "并发 $C - GET" $C 100000 "get" 8
done

# 4. 混合命令测试
echo "========== 4. 混合命令测试 =========="
run_test "混合命令 (P8)" 100 100000 "set,get,incr,lpush,rpush" 8

# 5. 大数据测试
echo "========== 5. 大数据测试 =========="
redis-benchmark -h 127.0.0.1 -p 6666 -a client123 \
    -c 50 -n 10000 -d 1024 -t set,get -q -P 8 2>/dev/null | grep -v "WARNING"
echo ""

redis-benchmark -h 127.0.0.1 -p 6666 -a client123 \
    -c 50 -n 10000 -d 10240 -t set,get -q -P 8 2>/dev/null | grep -v "WARNING"
echo ""

# 6. 长时稳定性测试
echo "========== 6. 稳定性测试 (30秒) =========="
echo "运行中..."
timeout 30 redis-benchmark -h 127.0.0.1 -p 6666 -a client123 \
    -c 50 -n 1000000 -t set -q -P 8 2>/dev/null | grep -v "WARNING"
echo ""

# 停止服务器
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "=========================================="
echo "压力测试完成！"
echo "=========================================="