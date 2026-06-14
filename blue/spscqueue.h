#pragma once
#include <memory>
#include <atomic>
#include <vector>

namespace blue
{

    //  SPSC 无锁队列
    template <typename T, size_t Capacity = 4096>
    class SPSCQueue
    {
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

    public:
        using SPSCQueuePtr = std::shared_ptr<SPSCQueue>;

    public:
        
        /**
         * @brief 构造函数
         */
        SPSCQueue() : m_buffer(Capacity), m_write(0), m_read(0) {}

        /**
         * @brief 构造函数
         * @param size 缓冲区大小
         */
        SPSCQueue(size_t size) : m_buffer(size) {}

        /**
         * @brief 生产者,加入元素,失败返回false
         * @param item 元素
         */
        bool push(const T &item)
        {
            size_t w = m_write.load(std::memory_order_acquire);
            size_t next = (w + 1) & (Capacity - 1);     // (w + 1) % Capacity
            if (next == m_read.load(std::memory_order_acquire))
            {
                return false; // 满了
            }
            m_buffer[w] = item;
            m_write.store(next, std::memory_order_release);
            return true;
        }

        /**
         * @brief 生产者：批量放入，返回实际放入数量
         * @param items 批量元素
         * @param count 大小
         */
        size_t pushBatch(const T *items, size_t count)
        {
            size_t pushed = 0;
            for (size_t i = 0; i < count; ++i)
            {
                if (!push(items[i]))
                {
                    break;
                }
                ++pushed;
            }
            return pushed;
        }

        /**
         * @brief 消费者：取出一个元素，失败返回 false（队列空）
         * @param item 取出的元素
         */
        bool pop(T &item)
        {
            size_t r = m_read.load(std::memory_order_acquire);
            size_t w = m_write.load(std::memory_order_acquire);
            if (r == w)
            {
                return false; // 空了
            }
            item = m_buffer[r];
            m_read.store((r + 1) & (Capacity - 1), std::memory_order_release);
            return true;
        }

        /**
         * @brief 消费者：批量取出，返回实际取出数量
         * @param items 批量取出的内容
         * @param maxCount 需要取出的大小
         */
        size_t popBatch(T *items, size_t maxCount)
        {
            size_t count = 0;
            for (size_t i = 0; i < maxCount; ++i)
            {
                if (!pop(items[i]))
                    break;
                ++count;
            }
            return count;
        }

        /**
         * @brief 是否为空
         */
        bool empty() const
        {
            return m_read.load(std::memory_order_acquire) == m_write.load(std::memory_order_acquire);
        }

        /**
         * @brief 是否满了
         */
        bool full() const
        {
            size_t w = m_write.load(std::memory_order_acquire);
            return ((w + 1) & (Capacity - 1)) == m_read.load(std::memory_order_acquire);
        }

    private:
        std::vector<T> m_buffer;
        std::atomic<size_t> m_write{0};
        std::atomic<size_t> m_read{0};
    };
}