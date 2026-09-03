#pragma once
#include <atomic>
#include <memory>
#include "blue/config.h"

namespace blue
{
    /**
     * @brief 热路径配置包装器：将 ConfigVar 与 std::atomic 绑定
     * @tparam T 配置类型（必须支持原子操作）
     */
    template <typename T>
    class AtomicConfigWrapper
    {
    public:
        using ConfigVarPtr = typename ConfigVar<T>::ConfigVarPtr;

    public:
        /**
         * @brief 构造并注册到配置系统
         * @param name 配置名称
         * @param default_val 默认值
         * @param description 描述
         */
        AtomicConfigWrapper(const std::string &name,
                            T default_val,
                            const std::string &description = "")
            : m_config(Config::Lookup(name, default_val, description)), m_atomic(default_val)
        {
            // 注册回调：当配置被修改时，同步更新原子变量
            m_callback_id = m_config->addListener(
                [this](const T & /*old_val*/, const T &new_val)
                {
                    m_atomic.store(new_val, std::memory_order_release);
                });
        }

        ~AtomicConfigWrapper()
        {
            if (m_config)
            {
                m_config->delListener(m_callback_id);
            }
        }

        AtomicConfigWrapper(const AtomicConfigWrapper &) = delete;
        AtomicConfigWrapper &operator=(const AtomicConfigWrapper &) = delete;

        AtomicConfigWrapper(AtomicConfigWrapper &&other) noexcept
            : m_config(std::move(other.m_config)), 
            m_atomic(other.m_atomic.load(std::memory_order_acquire)), 
            m_callback_id(other.m_callback_id)
        {
            other.m_callback_id = 0;
        }

        /**
         * @brief 热路径读取
         */
        inline T load() const noexcept
        {
            return m_atomic.load(std::memory_order_acquire);
        }

        /**
         * @brief 获取 ConfigVar 指针
         */
        ConfigVarPtr config() const { return m_config; }

        /**
         * @brief 手动设置值
         */
        void setValue(const T &val)
        {
            if (m_config)
            {
                m_config->setValue(val); // 回调会自动更新 m_atomic
            }
        }

    private:
        ConfigVarPtr m_config;
        std::atomic<T> m_atomic;
        uint64_t m_callback_id;
    };
}