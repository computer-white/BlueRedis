#pragma once
#ifdef COMMAND_TABLE
#include <array>
#include <algorithm>
#include <random>
#include "command_hash.h"

namespace blue
{
    template<typename T> class CommandHandler;
    template <size_t N> class CommandTable;

    using ArgValidator = bool(*)(size_t argc);
    using CommandHandlerFunc = blue::RespValue (*)(std::vector<RespValue> &,
                                              MSocket::MSocketPtr,
                                              bool,
                                              ServerData<int>& self);
    // 编译器命令表构建器
    template <size_t MaxCommands = 256>
    struct CommandTableBuilder
    {
        struct Entry
        {
            const char *name;
            blue::CommandHandlerFunc handler;
            uint32_t hash;
            bool is_write;
            ArgValidator argV;
        };

        Entry entries[MaxCommands];
        size_t count = 0;

        consteval CommandTableBuilder() = default;

        // 编译期插入
        consteval void insert(const char *name, blue::CommandHandlerFunc handler, uint32_t hash, bool write, ArgValidator argv)
        {
            entries[count] = {name, handler, hash, write, argv};
            count++;
        }

        // 编译期排序(插入排序)
        consteval void sort()
        {
            for (size_t i = 0; i < count; i++)
            {
                Entry key = entries[i];
                size_t j = i;
                while (j > 0 && entries[j - 1].hash > key.hash)
                {
                    entries[j] = entries[j - 1];
                    j--;
                }
                entries[j] = key;
            }
        }

        constexpr void swap(Entry &a, Entry &b)
        {
            Entry tem = std::move(a);
            a = std::move(b);
            b = std::move(tem);
        }

        constexpr size_t choosePivot(size_t left, size_t right)
        {
            // 三数取中：避免有序数组的最坏情况
            size_t mid = left + (right - left) / 2;

            // 比较三个候选值，选择中位数
            if (entries[left].hash > entries[mid].hash)
            {
                if (entries[mid].hash > entries[right].hash)
                    return mid;
                if (entries[left].hash > entries[right].hash)
                    return right;
                return left;
            }
            else
            {
                if (entries[left].hash > entries[right].hash)
                    return left;
                if (entries[mid].hash > entries[right].hash)
                    return right;
                return mid;
            }
        }

        constexpr size_t partition(size_t left, size_t right)
        {
            size_t pivot_idx = choosePivot(left, right);
            Entry pivot = entries[pivot_idx];
            swap(entries[pivot_idx], entries[right]);
            
            size_t i = left;
            for (size_t j = left; j < right; ++j)
            {
                if (entries[j].hash < pivot.hash) 
                {
                    swap(entries[i], entries[j]);
                    ++i;
                }
            }
            swap(entries[i], entries[right]);
            return i;
        }

        constexpr void process(size_t left, size_t right)
        {
            if (left >= right)
            {
                return;
            }
            int pivot = partition(left, right);
            if (pivot - left < right - pivot) 
            {
                process(left, pivot - 1);
                process(pivot + 1, right);
            } 
            else 
            {
                process(pivot + 1, right);
                process(left, pivot - 1);
            }
        }

        constexpr void quicksort()
        {
            // 无符号不要使用减号判断
            if (count > 1)
            {
                process(0, count - 1);
            }
        }

        consteval auto build()
        {
            quicksort();
            std::array<Entry, MaxCommands> result{};
            for (size_t i = 0; i < count; ++i) 
            {
                result[i] = entries[i];
            }
            return CommandTable<MaxCommands>(result, count);
        }
    };

    // 运行时命令表查找器
    template <size_t N>
    class CommandTable
    {
        public:
            using Entry = typename CommandTableBuilder<>::Entry;

            constexpr CommandTable(const std::array<Entry, N>& table, size_t count)
            : m_table(table), m_count(count) {}

            constexpr size_t lowerbound(uint32_t hash) const
            {
                size_t l = 0, r = m_count;
                while (l < r)
                {
                    // r 恒大于 l
                    size_t mid = l + (r - l) / 2;
                    if (m_table[mid].hash < hash)
                    {
                        l = mid + 1;
                    }
                    else
                    {
                        r = mid;
                    }
                }
                return r;
            }

            // 运行时二分(>= hash的第一个位置)
            constexpr const Entry* find_lowerbound(uint32_t hash) const 
            {
                if (m_count == 0)
                {
                    return nullptr;
                }
                size_t l = lowerbound(hash);
                if (l == m_count || m_table[l].hash != hash)
                {
                    return nullptr;
                }
                return &m_table[l];
            }

            constexpr size_t size() const { return m_count; }
            constexpr const Entry* begin() const { return m_table.data(); }
            constexpr const Entry* end() const { return m_table.data() + m_count; }

        private:
            std::array<Entry, N> m_table;
            size_t m_count;
    };

}
#else
#endif
