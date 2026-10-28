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
 * @file defer.h
 * @brief 类似go 的 defer 对资源的自动析构
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.9.18
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <functional>
#include <utility>

namespace blue
{
    /**
     * @brief RAII 风格的 defer,作用域退出时执行清理
     *
     * 用法:
     *   defer(close(fd));
     *   defer({ std::cout << "cleanup\n"; free(p); });
     *
     * 语义:
     *   - 作用域正常退出时执行
     *   - 异常退出时也执行
     *   - 多个 Defer 按构造逆序析构,实现 LIFO
     *   - 析构里不能抛异常(内部吞掉)
     *
     * @note 协程里也适用:协程挂起期间 Defer 对象仍在协程帧里,
     *       协程结束(正常/异常)时触发清理。
     */
    class Defer
    {
    public:
        explicit Defer(std::function<void()> fn) : fn_(std::move(fn)) {}

        ~Defer()
        {
            if (fn_)
            {
                try
                {
                    fn_();
                }
                catch (...)
                {
                    // 吞掉
                }
            }
        }

        Defer(const Defer &) = delete;
        Defer &operator=(const Defer &) = delete;

        Defer(Defer &&other) noexcept : fn_(std::move(other.fn_))
        {
            other.fn_ = nullptr;
        }
        Defer &operator=(Defer &&) = delete;

    private:
        std::function<void()> fn_;
    };
}
#define BLUE_DEFER_CONCAT_(a, b) a##b
#define BLUE_DEFER_CONCAT(a, b) BLUE_DEFER_CONCAT_(a, b)

/**
 * @brief 注册一个作用域退出时执行的清理代码
 *
 * @code
 *   int fd = open(...);
 *   defer(close(fd));
 * @endcode
 */
#define defer(code) ::blue::Defer BLUE_DEFER_CONCAT(_defer_, __LINE__){[&]{ code; }}