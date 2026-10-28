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
 * @file memory_pool.h
 * @brief 内存池（无锁，单生产者单消费者）
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.26
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <memory>
#include <atomic>
#include <vector>
#include <cstdlib>

namespace blue
{
    template <typename T, size_t Capacity = 4096>
    class MemoryPoolNoUnique
    {
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

    public:
        MemoryPoolNoUnique() : m_pool(Capacity, nullptr), m_size(Capacity)
        {
            for (size_t i = 0; i < Capacity; i++)
            {
                m_pool[i] = static_cast<T *>(malloc(sizeof(T)));
            }
        }

        ~MemoryPoolNoUnique()
        {
            for (size_t i = 0; i < Capacity; i++)
            {
                if (m_pool[i])
                {
                    free((void *)m_pool[i]);
                }
            }
        }

        MemoryPoolNoUnique(const MemoryPoolNoUnique &lhs) = delete;
        MemoryPoolNoUnique &operator=(const MemoryPoolNoUnique &lhs) = delete;

        MemoryPoolNoUnique(MemoryPoolNoUnique &&rhs) noexcept
            : m_pool(std::move(rhs.m_pool)),
              m_product(rhs.m_product.load(std::memory_order_acquire)),
              m_consumer(rhs.m_consumer.load(std::memory_order_acquire)),
              m_size(rhs.m_size.load(std::memory_order_acquire))
        {
            rhs.m_product.store(0, std::memory_order_release);
            rhs.m_consumer.store(0, std::memory_order_release);
            rhs.m_size.store(0, std::memory_order_release);
        }

        MemoryPoolNoUnique &operator=(MemoryPoolNoUnique &&rhs) noexcept
        {
            if (this != &rhs)
            {
                for (size_t i = 0; i < Capacity; i++)
                {
                    if (m_pool[i])
                    {
                        free((void *)m_pool[i]);
                    }
                }

                m_pool = std::move(rhs);
                m_product.store(rhs.m_product.load(std::memory_order_acquire), std::memory_order_release);
                m_consumer.store(rhs.m_consumer.load(std::memory_order_acquire), std::memory_order_release);
                m_size.store(rhs.m_size.load(std::memory_order_acquire), std::memory_order_release);

                rhs.m_product.store(0, std::memory_order_release);
                rhs.m_consumer.store(0, std::memory_order_release);
                rhs.m_size.store(0, std::memory_order_release);
            }
            return *this;
        }

        // 获取一个对象
        T *acquire()
        {
            if (m_size.load(std::memory_order_acquire) == 0)
            {
                return nullptr;
            }

            size_t c = m_consumer.load(std::memory_order_acquire);
            T *item = m_pool[c];
            m_consumer.store((c + 1) & (Capacity - 1), std::memory_order_release);
            m_size.fetch_sub(1, std::memory_order_release);
            return item;
        }

        // 归还一个对象
        bool release(T *item)
        {
            if (!item)
            {
                return false;
            }

            if (m_size.load(std::memory_order_acquire) >= Capacity)
            {
                return false;
            }

            size_t p = m_product.load(std::memory_order_acquire);
            m_pool[p] = item;
            m_product.store((p + 1) & (Capacity - 1), std::memory_order_release);
            m_size.fetch_add(1, std::memory_order_release);
            return true;
        }

        // 批量获取
        std::vector<T *> acquire_batch(size_t count)
        {
            std::vector<T *> result;
            result.reserve(count);

            for (size_t i = 0; i < count; i++)
            {
                auto ptr = acquire();
                if (ptr)
                {
                    result.push_back(ptr);
                }
                else
                {
                    break;
                }
            }
            return result;
        }

        // 批量归还
        size_t release_batch(std::vector<T *> &items)
        {
            size_t released = 0;
            for (auto &ptr : items)
            {
                if (ptr && release(ptr))
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
        std::vector<T *> m_pool;
        std::atomic<size_t> m_product{0};     // 放的位置
        std::atomic<size_t> m_consumer{0};    // 取的位置
        std::atomic<size_t> m_size{Capacity}; // 当前对象数量
    };

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

        MemoryPool(const MemoryPool &lhs) = delete;
        MemoryPool &operator=(const MemoryPool &lhs) = delete;

        MemoryPool(MemoryPool &&rhs) noexcept
            : m_pool(std::move(rhs.m_pool)),
              m_consumer(rhs.m_consumer.load(std::memory_order_acquire)),
              m_product(rhs.m_product.load(std::memory_order_acquire)),
              m_size(rhs.m_size.load(std::memory_order_acquire))
        {
            rhs.m_consumer.store(0, std::memory_order_release);
            rhs.m_product.store(0, std::memory_order_release);
            rhs.m_size.store(0, std::memory_order_release);
        }

        MemoryPool &operator=(MemoryPool &&rhs) noexcept
        {
            if (this != &rhs)
            {
                m_pool = std::move(rhs.m_pool);
                m_product.store(rhs.m_product.load(std::memory_order_acquire), std::memory_order_release);
                m_consumer.store(rhs.m_consumer.load(std::memory_order_acquire), std::memory_order_release);
                m_size.store(rhs.m_size.load(std::memory_order_acquire), std::memory_order_release);

                rhs.m_product.store(0, std::memory_order_release);
                rhs.m_consumer.store(0, std::memory_order_release);
                rhs.m_size.store(0, std::memory_order_release);
            }
            return *this;
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

            if (m_size.load(std::memory_order_acquire) >= Capacity)
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