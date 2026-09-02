/*
 * BlueRedis - High Performance Redis Server based on C++20 Coroutine
 * Copyright (C) 2026 blue
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/**
 * @file skiplist.h
 * @brief 跳表数据结构的实现
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.5
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <vector>
#include <random>
#include <string>
#include <limits>
#include "blue/memory_pool.h"

namespace blue
{
    template <typename K, typename V>
    class SkipList
    {
    public:
        static constexpr int MAX_LEVEL = 16;
        static constexpr double P = 0.25;
        struct Node
        {
            K key;
            V val;
            Node *forward[MAX_LEVEL]; // 链表的next
            int span[MAX_LEVEL];      // 跨度：到下一个节点的距离
            Node(const K &k = K(), const V &v = V())
                : key(k), val(v)
            {
                for (int i = 0; i < MAX_LEVEL; ++i)
                {
                    forward[i] = nullptr;
                    span[i] = 0;
                }
            }

            ~Node()
            {
                for (int i = 0; i < MAX_LEVEL; ++i)
                {
                    if (forward[i])
                    {
                        free((void *)forward[i]);
                    }
                    span[i] = 0;
                }
            }

            void reset()
            {
                for (int i = 0; i < MAX_LEVEL; ++i)
                {
                    forward[i] = nullptr;
                    span[i] = 0;
                }
            }
        };
        using NodeType = Node;

    public:
        /**
         * @brief 构造函数（使用内存池）
         */
        SkipList()
            : m_pool(new MemoryPoolNoUnique<SkipList<K, V>::Node, 4096>()),
              m_level(1),
              m_size(0)
        {
            // 分配头节点
            if (m_pool)
            {
                m_head = m_pool->acquire();
                if (m_head)
                {
                    // placement new 构造
                    // 在m_head内存上分配对象Node
                    new (m_head) Node();
                }
                else
                {
                    // 池为空，回退到 new
                    m_head = new Node();
                }
            }
            else
            {
                m_head = new Node();
            }

            if (m_head)
            {
                for (int i = 0; i < MAX_LEVEL; ++i)
                {
                    m_head->forward[i] = nullptr;
                    m_head->span[i] = 0;
                }
            }
        }

        SkipList(const SkipList &) = delete;
        SkipList &operator=(const SkipList &) = delete;

        SkipList(SkipList &&other) noexcept
            : m_pool(std::move(other.m_pool)), m_head(other.m_head), m_level(other.m_level), m_size(other.m_size)
        {
            other.m_head = nullptr;
            other.m_level = 1;
            other.m_size = 0;
        }

        SkipList &operator=(SkipList &&other) noexcept
        {
            if (this != &other) // 比较地址
            {
                // 清理自己
                this->clear();

                // 接管 other
                m_pool = std::move(other.m_pool);
                m_head = other.m_head;
                m_level = other.m_level;
                m_size = other.m_size;

                other.m_head = nullptr;
                other.m_level = 1;
                other.m_size = 0;
            }
            return *this;
        }

        ~SkipList()
        {
            this->clear();
        }

        /**
         * @brief 清空所有节点
         */
        void clear()
        {
            if (!m_head)
            {
                return;
            }

            Node *node = m_head->forward[0];
            while (node)
            {
                Node *next = node->forward[0];
                deallocateNode(node);
                node = next;
            }

            deallocateNode(m_head);
            m_head = nullptr;
            m_level = 1;
            m_size = 0;
        }

        /*
          span:4            span:4                    level
            0 -------------- 20 -----------------30     2
            |                 |                   |
            | s:2      s:2    | s:2     s:2       |
            0 --------10------20 ------25---------30    1
            |         |       |         |         |
            |         |       |         |         |
全部s = 1   0_---5----10--15--20---24---25---27---30    0
        */

        void insert(const K &key, const V &val)
        {
            Node *update[MAX_LEVEL];
            int rank[MAX_LEVEL];
            Node *curr = m_head;
            for (int i = m_level - 1; i >= 0; i--)
            {
                rank[i] = i == m_level - 1 ? 0 : rank[i + 1]; // 继承上一层的排名
                while (curr->forward[i] && curr->forward[i]->key < key)
                {
                    rank[i] += curr->span[i]; // 拿到当前i层的key的排名
                    curr = curr->forward[i];
                }
                update[i] = curr; // update[i]指向要插入key的位置的上一个位置
            }

            curr = curr->forward[0];
            if (curr && curr->key == key)
            {
                curr->val = val;
                return;
            }

            int newlevel = randomLevel();
            int old_level = m_level; // 保存old_level,假如随机出来的new_level比old_level小，需要更新new_level到old_level的Span
            if (newlevel > m_level)
            {
                for (int i = m_level; i < newlevel; i++)
                {
                    rank[i] = 0;
                    update[i] = m_head;
                    update[i]->span[i] = m_size;
                }
                m_level = newlevel;
            }

            Node *node = this->allocateNode(key, val);
            if (!node)
            {
                node = new Node(key, val);
            }
            for (int i = 0; i < newlevel; i++)
            {
                node->forward[i] = update[i]->forward[i];
                update[i]->forward[i] = node;

                // 更新span
                /*
                    在上面的图中，加入插入17,那么update[i]->spac[i]可能的取值有15，10，0的span
                    rank[0]表示到最后的15的累加排名，其实也就是15的排名,rank[i]表示从第level-1层到第i层插入节点前一个节点的累加排名

                    i = 0,那么updata[0]->span[0] = 15->span[0] = 1,rank[0] = sum(到15->span[0]) = 3,rank[i] = 累加到(到插入节点
                    17之前的15的span[0]) = 3(15的排名),所以node->span[0] = 1,update[0]->span[i] = 3 - 3 + 1 = 1，即就是15到17的跨度
                    i = 1,那么update[1]->span[1] = 10->span[1] = 2,rank[0]不变=3,rank[i] = 一直累加到(10->span[i]),也就是10的排名=2
                    所以node->span[1] = 1,update[1]->span[1] = (3 - 2) + 1 = 2,也就是从10到17
                */
                node->span[i] = update[i]->span[i] - (rank[0] - rank[i]);
                update[i]->span[i] = (rank[0] - rank[i]) + 1;
            }

            for (int i = newlevel; i < old_level; i++)
            {
                update[i]->span[i]++;
            }
            m_size++;
        }

        bool remove(const K &key)
        {
            Node *update[MAX_LEVEL];
            int rank[MAX_LEVEL];
            Node *curr = m_head;
            for (int i = m_level - 1; i >= 0; i--)
            {
                rank[i] = (i == m_level - 1) ? 0 : rank[i + 1];
                while (curr->forward[i] && curr->forward[i]->key < key)
                {
                    rank[i] += curr->span[i]; // 拿到到目前i层的key的排名
                    curr = curr->forward[i];
                }
                update[i] = curr; // 指向要删除位置的前一个位置
            }

            curr = curr->forward[0];
            if (!curr || curr->key != key)
            {
                return false;
            }
            for (int i = 0; i < m_level; i++)
            {
                if (update[i]->forward[i] != curr)
                {
                    update[i]->span[i]--; // 下一个不是删除的节点，但是删除的节点处在update[i] 到 curr之间，所以要减一
                }
                else
                {
                    update[i]->forward[i] = curr->forward[i];
                    update[i]->span[i] += curr->span[i] - 1; // 更新删除后的span
                }
            }

            this->deallocateNode(curr);

            while (m_level > 1 && m_head->forward[m_level - 1] == nullptr)
            {
                m_level--;
            }

            m_size--;
            return true;
        }

        NodeType *find(const K &key) const
        {
            Node *curr = m_head;
            for (int i = m_level - 1; i >= 0; i--)
            {
                while (curr->forward[i] && curr->forward[i]->key < key)
                {
                    curr = curr->forward[i];
                }
            }
            curr = curr->forward[0];
            if (curr && curr->key == key)
            {
                return curr;
            }
            return nullptr;
        }

        NodeType *getByIndex(int index) const
        {
            if (index < 0 || index >= m_size)
            {
                return nullptr;
            }
            Node *curr = m_head->forward[0];
            for (int i = 0; i < index; i++)
            {
                curr = curr->forward[0];
            }
            return curr;
        }

        // 获取排名（用于 ZRANK）
        int getRank(const K &key) const
        {
            int rank = 0;
            Node *curr = m_head;
            for (int i = m_level - 1; i >= 0; i--)
            {
                while (curr->forward[i] && curr->forward[i]->key < key)
                {
                    rank += curr->span[i];
                    curr = curr->forward[i];
                }
            }
            curr = curr->forward[0];
            if (curr && curr->key == key)
            {
                return rank;
            }
            return -1;
        }

        /**
         * @brief 获取有序集合的大小
         */
        size_t size() const { return m_size; }

        /**
         * @brief 判断是否为空
         */
        bool empty() const { return m_size == 0; }

        /**
         * @brief 获取头节点的第一个元素（用于遍历）
         */
        NodeType *begin() const { return m_head ? m_head->forward[0] : nullptr; }

    private:
        int randomLevel()
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_real_distribution<> dis(0.0, 1.0);

            int level = 1;
            while (dis(gen) < P && level < MAX_LEVEL)
            {
                level++;
            }
            return level;
        }

        /**
         * @brief 从内存池分配节点并构造
         */
        Node *allocateNode(const K &key, const V &val)
        {
            if (!m_pool)
            {
                return nullptr;
            }

            Node *node = m_pool->acquire();
            if (node)
            {
                // placement new 构造对象
                new (node) Node(key, val);
            }
            return node;
        }

        /**
         * @brief 释放节点回内存池
         */
        void deallocateNode(Node *node)
        {
            if (!node)
            {
                return;
            }

            node->reset();

            if (m_pool)
            {
                m_pool->release(node);
            }
            else
            {
                delete node;
            }
        }

    private:
        MemoryPoolNoUnique<Node> *m_pool = nullptr;
        Node *m_head = nullptr;
        int m_level = 1; // 当前层级
        size_t m_size = 0;
    };

    struct ZSetKey
    {
        double score;
        std::string member;
        ZSetKey() : score(0.0) {}
        ZSetKey(double sco, std::string mem)
            : score(sco), member(std::move(mem)) {}
        bool operator<(const ZSetKey &other) const
        {
            if (score != other.score)
            {
                return score < other.score;
            }
            return member < other.member;
        }

        bool operator==(const ZSetKey &other) const
        {
            return score == other.score && member == other.member;
        }

        bool operator!=(const ZSetKey &other) const
        {
            return !(*this == other);
        }
    };
}