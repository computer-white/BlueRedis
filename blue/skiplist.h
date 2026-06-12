#pragma once
#include <vector>
#include <random>
#include <string>
#include <limits>

namespace blue
{
    template <typename K, typename V>
    class SkipList
    {
    public:
        struct Node
        {
            K key;
            V val;
            std::vector<Node *> forward;
            Node(K k, V v, int level)
                : key(k), val(v), forward(level, nullptr) {}
        };
        using NodeType = Node;
    public:
        SkipList()
            : m_head(new Node(K{}, V{}, MAX_LEVEL)),
              m_level(1)
        {
            m_head->forward.assign(MAX_LEVEL, nullptr);
        }

        SkipList(const SkipList&) = delete;
        SkipList& operator=(const SkipList&) = delete;

        SkipList(SkipList&& other) noexcept
            : m_head(other.m_head)
            , m_level(other.m_level)
            , m_size(other.m_size)
        {
            other.m_head = nullptr;
            other.m_level = 1;
            other.m_size = 0;
        }

        SkipList& operator=(SkipList&& other) noexcept
        {
            if (this != &other)     // 比较地址
            {
                // 清理自己
                Node* node = m_head->forward[0];
                while (node)
                {
                    Node* next = node->forward[0];
                    delete node;
                    node = next;
                }
                delete m_head;
                
                // 接管 other
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
            if (!m_head)
            {
                return;
            }
            Node *node = m_head->forward[0];
            while (node)
            {
                Node *nxt = node->forward[0];
                delete node;
                node = nxt;
            }
            delete m_head;
        }

        void insert(const K &key, const V &val)
        {
            std::vector<Node *> update(MAX_LEVEL, nullptr);
            Node *curr = m_head;
            for (int i = m_level - 1; i >= 0; i--)
            {
                while (curr->forward[i] && curr->forward[i]->key < key)
                {
                    curr = curr->forward[i];
                }
                update[i] = curr;
            }

            curr = curr->forward[0];
            if (curr && curr->key == key)
            {
                curr->val = val;
                return;
            }

            int newlevel = randomLevel();
            if (newlevel > m_level)
            {
                for (int i = m_level; i < newlevel; i++)
                {
                    update[i] = m_head;
                }
                m_level = newlevel;
            }

            Node *node = new Node(key, val, newlevel);
            for (int i = 0; i < newlevel; i++)
            {
                node->forward[i] = update[i]->forward[i];
                update[i]->forward[i] = node;
            }

            m_size++;
        }

        bool remove(const K &key)
        {
            std::vector<Node *> update(MAX_LEVEL, nullptr);
            Node *curr = m_head;
            for (int i = m_level - 1; i >= 0; i--)
            {
                while (curr->forward[i] && curr->forward[i]->key < key)
                {
                    curr = curr->forward[i];
                }
                update[i] = curr;
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
                    break;
                }
                update[i]->forward[i] = curr->forward[i];
            }

            delete curr;

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
                    rank += (1 << i);
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

        size_t size() const { return m_size; }
        bool empty() const { return m_size == 0; }
        NodeType *begin() const { return m_head->forward[0]; }

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

    private:
        static constexpr int MAX_LEVEL = 16;
        static constexpr double P = 0.25;
        Node *m_head;
        int m_level; // 当前层级
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