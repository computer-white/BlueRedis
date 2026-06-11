#!/bin/bash
# test_full.sh

echo "=== Complete Test with Single Connection ==="

./bin/your_server &
SERVER_PID=$!
sleep 2

# 使用一个持久连接测试所有命令
redis-cli -p 6666 << 'EOF'
PING
SET test hello
GET test
DEL test
GET test

LPUSH mylist a
LPUSH mylist b
LRANGE mylist 0 -1
LPOP mylist
LRANGE mylist 0 -1
RPUSH mylist c
RPUSH mylist d
LRANGE mylist 0 -1

HSET user name alice
HGET user name
HSET user age 30
HGETALL user
HDEL user name
HGETALL user

INCR counter
INCR counter
INCRBY counter 10
GET counter

ZADD zset 1 one
ZADD zset 2 two
ZRANGE zset 0 -1 WITHSCORES
ZSCORE zset one
ZRANK zset two

EOF

kill $SERVER_PID