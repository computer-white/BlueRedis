#!/bin/bash

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Redis 连接配置
REDIS_CMD="redis-cli -p 6666"
AUTH_CMD="auth admin123"

# 执行带认证的命令
redis_cmd() {
    $REDIS_CMD <<EOF
$AUTH_CMD
$1
EOF
}

# 执行多个命令
redis_cmds() {
    $REDIS_CMD <<EOF
$AUTH_CMD
$1
EOF
}

echo -e "${GREEN}=== 开始测试 SLOWLOG 功能 ===${NC}\n"

# 1. 设置慢查询阈值
echo -e "${YELLOW}1. 设置慢查询阈值为 1 微秒${NC}"
redis_cmd "CONFIG SET slowlog-log-slower-than 1"
redis_cmd "CONFIG SET slowlog-max-len 128"
echo ""

# 2. 重置慢查询
echo -e "${YELLOW}2. 重置慢查询日志${NC}"
redis_cmd "SLOWLOG RESET"
echo ""

# 3. 测试字符串命令
echo -e "${YELLOW}3. 测试字符串命令${NC}"
redis_cmd "SET str_key 'Hello World'"
redis_cmd "GET str_key"
redis_cmd "APPEND str_key ' appended'"
redis_cmd "STRLEN str_key"
redis_cmd "GETSET str_key 'New Value'"
echo ""

# 4. 测试哈希命令
echo -e "${YELLOW}4. 测试哈希命令${NC}"
redis_cmd "HSET hash_key field1 value1 field2 value2 field3 value3"
redis_cmd "HGET hash_key field1"
redis_cmd "HGETALL hash_key"
redis_cmd "HLEN hash_key"
redis_cmd "HEXISTS hash_key field2"
redis_cmd "HKEYS hash_key"
redis_cmd "HVALS hash_key"
echo ""

# 5. 测试列表命令
echo -e "${YELLOW}5. 测试列表命令${NC}"
redis_cmd "LPUSH list_key a b c d e"
redis_cmd "RPUSH list_key f g h"
redis_cmd "LRANGE list_key 0 -1"
redis_cmd "LLEN list_key"
redis_cmd "LINDEX list_key 2"
redis_cmd "LSET list_key 1 new_value"
redis_cmd "LINSERT list_key BEFORE d insert_value"
redis_cmd "LPOP list_key"
redis_cmd "RPOP list_key"
echo ""

# 6. 测试集合命令
echo -e "${YELLOW}6. 测试集合命令${NC}"
redis_cmd "SADD set_key a b c d e f"
redis_cmd "SMEMBERS set_key"
redis_cmd "SISMEMBER set_key a"
redis_cmd "SISMEMBER set_key z"
redis_cmd "SCARD set_key"
redis_cmd "SREM set_key a b"
redis_cmd "SADD set2 x y z"
redis_cmd "SUNION set_key set2"
redis_cmd "SINTER set_key set2"
redis_cmd "SDIFF set_key set2"
echo ""

# 7. 测试有序集合命令
echo -e "${YELLOW}7. 测试有序集合命令${NC}"
redis_cmd "ZADD zset_key 10 member1 20 member2 30 member3 40 member4 50 member5"
redis_cmd "ZRANGE zset_key 0 -1 WITHSCORES"
redis_cmd "ZSCORE zset_key member3"
redis_cmd "ZRANK zset_key member2"
redis_cmd "ZCOUNT zset_key 15 35"
redis_cmd "ZINCRBY zset_key 5 member3"
redis_cmd "ZRANGEBYSCORE zset_key 20 40"
redis_cmd "ZREMRANGEBYSCORE zset_key 25 35"
redis_cmd "ZREM zset_key member1"
echo ""

# 8. 测试事务模式
echo -e "${YELLOW}8. 测试事务模式${NC}"
redis_cmds "
MULTI
SET tx_key1 tx_value1
SET tx_key2 tx_value2
GET tx_key1
EXEC
"
echo ""

# 测试 DISCARD
echo -e "${YELLOW}9. 测试事务 DISCARD${NC}"
redis_cmds "
MULTI
SET tx_key3 tx_value3
SET tx_key4 tx_value4
DISCARD
"
echo ""

# 9. 测试订阅模式
echo -e "${YELLOW}10. 测试订阅模式${NC}"
# 注意：SUBSCRIBE 会阻塞，所以我们用 PSUBSCRIBE 或者直接测试 PUBLISH
redis_cmd "PUBLISH test_channel 'Hello from test'"
echo ""

# 10. 测试键管理命令
echo -e "${YELLOW}11. 测试键管理命令${NC}"
redis_cmd "EXISTS str_key hash_key list_key set_key zset_key"
redis_cmd "KEYS *"
redis_cmd "RANDOMKEY"
redis_cmd "DBSIZE"
redis_cmd "TYPE str_key"
redis_cmd "TYPE hash_key"
redis_cmd "TYPE list_key"
redis_cmd "TYPE set_key"
redis_cmd "TYPE zset_key"
echo ""

# 11. 测试过期命令
echo -e "${YELLOW}12. 测试过期命令${NC}"
redis_cmd "EXPIRE str_key 60"
redis_cmd "TTL str_key"
redis_cmd "PEXPIRE str_key 60000"
redis_cmd "PTTL str_key"
redis_cmd "PERSIST str_key"
echo ""

# 12. 测试重命名命令
echo -e "${YELLOW}13. 测试重命名命令${NC}"
redis_cmd "RENAME str_key str_key_renamed"
redis_cmd "RENAMENX str_key_renamed str_key_new"
echo ""

# 13. 测试其他命令
echo -e "${YELLOW}14. 测试其他命令${NC}"
redis_cmd "PING"
redis_cmd "ECHO 'Hello Redis'"
redis_cmd "TIME"
redis_cmd "LOCALTIME"
redis_cmd "INFO"
echo ""

# 14. 查看慢查询日志
echo -e "${GREEN}=== 15. 慢查询日志 (SLOWLOG GET) ===${NC}"
redis_cmd "SLOWLOG GET"
echo ""

echo -e "${GREEN}=== 16. 慢查询日志长度 (SLOWLOG LEN) ===${NC}"
redis_cmd "SLOWLOG LEN"
echo ""

# 15. 查看配置
echo -e "${GREEN}=== 17. 慢查询配置 (CONFIG GET) ===${NC}"
redis_cmd "CONFIG GET slowlog-*"
echo ""

# 16. 重置慢查询
echo -e "${YELLOW}18. 重置慢查询日志${NC}"
redis_cmd "SLOWLOG RESET"
redis_cmd "SLOWLOG LEN"
echo ""

echo -e "${GREEN}=== 测试完成 ===${NC}"