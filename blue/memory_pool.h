#pragma once
#include <memory>
#include <atomic>
#include <vector>

namespace blue
{
    template <typename T, size_t Capacity = 4096>
    class MemoryPool
    {
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

    public:
        MemoryPool() : m_pool(Capacity), m_size(Capacity)
        {
            for (size_t i = 0; i < Capacity; i++)
            {
                m_pool[i] = std::make_unique<T>();
            }
        }

        // 获取一个对象
        std::unique_ptr<T> acquire()
        {
            if (m_size.load(std::memory_order_acquire) == 0)
            {
                return nullptr;
            }

            size_t c = m_consumer.load(std::memory_order_acquire);
            auto item = std::move(m_pool[c]);
            m_consumer.store((c + 1) & (Capacity - 1), std::memory_order_release);
            m_size.fetch_sub(1, std::memory_order_release);
            return item;
        }

        // 归还一个对象
        bool release(std::unique_ptr<T> item)
        {
            if (!item)
            {
                return false;
            }

            if (m_size.load(std::memory_order_acquire) == Capacity)
            {
                return false;
            }

            size_t p = m_product.load(std::memory_order_acquire);
            m_pool[p] = std::move(item);
            m_product.store((p + 1) & (Capacity - 1), std::memory_order_release);
            m_size.fetch_add(1, std::memory_order_release);
            return true;
        }

        // 批量获取
        std::vector<std::unique_ptr<T>> acquire_batch(size_t count)
        {
            std::vector<std::unique_ptr<T>> result;
            result.reserve(count);

            for (size_t i = 0; i < count; i++)
            {
                auto ptr = acquire();
                if (ptr)
                {
                    result.push_back(std::move(ptr));
                }
                else
                {
                    break;
                }
            }
            return result;
        }

        // 批量归还
        size_t release_batch(std::vector<std::unique_ptr<T>> &items)
        {
            size_t released = 0;
            for (auto &ptr : items)
            {
                if (ptr && release(std::move(ptr)))
                {
                    released++;
                }
                else
                {
                    break;
                }
            }
            return released;
        }

        bool empty() const { return m_size.load(std::memory_order_acquire) == 0; }
        bool full() const { return m_size.load(std::memory_order_acquire) == Capacity; }
        size_t capacity() const { return Capacity; }
        size_t size() const { return m_size.load(std::memory_order_acquire); }

    private:
        std::vector<std::unique_ptr<T>> m_pool;
        std::atomic<size_t> m_product{0};     // 放的位置
        std::atomic<size_t> m_consumer{0};    // 取的位置
        std::atomic<size_t> m_size{Capacity}; // 当前对象数量
    };
}