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
 * @file task.h
 * @brief 核心模块，对于协程任务的实现，协程之间采用对称转移
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.1
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <coroutine>
#include <exception>
#include <utility>
#include <memory>
#include <variant>
#include <type_traits>
#include <functional>
#include <atomic>

namespace blue
{
    template <typename T>
    struct Task;

    /**
     * @brief 子协程 final_suspend 的返回类型，用于对称转移
     */
    struct SubCorroutine
    {
        std::coroutine_handle<> fa;

        bool await_ready() const noexcept { return false; }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const noexcept
        {
            if (fa && fa.address() && !fa.done())
            {
                return fa;
            }
            return std::noop_coroutine();
        }

        void await_resume() const noexcept {}

        explicit SubCorroutine(std::coroutine_handle<> p = nullptr) : fa(p) {}
    };

    /**
     * @brief 类型擦除基类：让调度器能持有任意 Task<T>
     */
    struct TaskBase
    {
        virtual ~TaskBase() = default;
        virtual void resume() = 0;
        virtual bool done() const noexcept = 0;
        // 回调函数，用于在schedule提交了一个协程后设置回调
        // 这个被设置的回调函数会在final_suspend中被调用，用于将已经完成的协程转移到final列表
        virtual void setOnComplete(std::function<void()>) = 0;
        // 多线程下有竞争(UAF),当调度器清理顶层协程帧时，这个协程虽然可能已经 done，但是正在final_suspend里面
        // 若外部引用计数为0后，会destroy这个协程帧
        // 在promise_type中保存一个原子变量，这个函数用于外部调度器可以拿到那个原子变量的值
        // 目的是为了不让协程帧在 final_suspend 中还未退出，就被外部销毁
        virtual bool finalizing() const noexcept = 0;
    };

    namespace detail
    {
        // 有返回值：提供 return_value
        template <typename T>
        struct PromiseReturnValue
        {
            T val{};
            void return_value(T v) { val = std::move(v); }
        };

        // void：提供 return_void
        template <>
        struct PromiseReturnValue<void>
        {
            void return_void() {}
        };
    }

    /**
     * @brief 协程任务模板（统一处理 T 和 void）
     */
    template <typename T = void>
    struct Task
    {
        struct promise_type : detail::PromiseReturnValue<T>
        {
            std::coroutine_handle<> fa = nullptr; // 父协程句柄
            std::exception_ptr exception = nullptr;
            // 回调函数：调用在shcedule中设置的回调函数，通常是将已经完成的协程 Task 转移到 finished list中
            // 这是将协程帧的生命周期交给外部定时器的重要设计
            std::function<void()> on_complete = nullptr;
            // 用于确保在进入 final_suspend 之后和成功调用完cb之前 保证不去清理 Task 的协程帧
            // 这是协程和调度器以及定时器生命周期管理的最后一块拼图，用于防止多线程下竟态导致的UAF
            std::atomic<bool> finalizing{false};

            Task get_return_object()
            {
                // 后续可以加上协程内存池来减少每次协程创建都要申请内存的开销
                return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() { return {}; }

            SubCorroutine final_suspend() noexcept
            {
                // [架构设计：防止 final_suspend 阶段的 UAF]
                // 协程进入 final_suspend 后，其内部逻辑（如执行 on_complete 回调）依然在协程栈上运行。
                // 此时如果外部调度器释放了该协程的最后一个引用（shared_ptr），会导致当前执行流所在的内存被销毁。
                // 通过原子变量 finalizing 设置“免死金牌”，确保调度器在清理时，必须等到此函数完全返回后才能析构协程帧。
                
                // 标记：帧已进入 final suspend，但 final_suspend 还没返回
                finalizing.store(true, std::memory_order_release);
                if (on_complete)
                {
                    auto cb = std::move(on_complete);
                    on_complete = nullptr;
                    // cb 里面永远不做删除帧的操作，只做转移
                    cb();
                }
                finalizing.store(false, std::memory_order_release);
                return SubCorroutine{fa};
            }

            void unhandled_exception() { exception = std::current_exception(); }
        };

        using HandleType = std::coroutine_handle<promise_type>;

        Task() : handle(nullptr) {}
        explicit Task(HandleType h) : handle(h) {}

        ~Task() { destroySafe(); }

        Task(const Task &) = delete;
        Task &operator=(const Task &) = delete;

        Task(Task &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

        Task &operator=(Task &&other) noexcept
        {
            if (this != &other)
            {
                destroySafe();
                handle = std::exchange(other.handle, nullptr);
            }
            return *this;
        }

        bool done() const noexcept { return !handle || handle.done(); }

        void resume() const
        {
            if (handle && !handle.done())
            {
                handle.resume();
            }
        }

        // 有返回值
        template <typename U = T>
            requires(!std::is_void_v<U>)
        U get() const
        {
            if (handle && handle.promise().exception)
            {
                std::rethrow_exception(handle.promise().exception);
            }
            return std::move(handle.promise().val);
        }

        HandleType getHandle() const noexcept { return handle; }
        std::coroutine_handle<> getHandleNoType() const noexcept { return handle; }

        void destroy() { destroySafe(); }

        explicit operator bool() const noexcept { return handle != nullptr; }

        Task &operator=(std::nullptr_t) noexcept
        {
            destroySafe();
            handle = nullptr;
            return *this;
        }

        bool await_ready() const noexcept { return !handle || handle.done(); }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> call) const noexcept
        {
            if (handle && !handle.done())
            {
                handle.promise().fa = call;
                return handle;
            }
            return std::noop_coroutine();
        }

        // 有返回值
        template <typename U = T>
            requires(!std::is_void_v<U>)
        U await_resume() const
        {
            if (handle && handle.promise().exception)
            {
                std::rethrow_exception(handle.promise().exception);
            }
            return std::move(handle.promise().val);
        }

        // void 返回值
        template <typename U = T>
            requires std::is_void_v<U>
        void await_resume() const
        {
            if (handle && handle.promise().exception)
            {
                std::rethrow_exception(handle.promise().exception);
            }
        }

    private:
        HandleType handle;

        void destroySafe()
        {
            if (!handle)
            {
                return;
            }
            HandleType h = std::exchange(handle, nullptr);

            // 销毁已完成的帧
            if (h.done())
            {
                h.destroy();
                return;
            }
            // 在目前的设计中，帧没有完成就代表他没有被执行，只是被声明定义了
            // 可以安全的destroy()
            h.destroy();
        }
    };

    /**
     * @brief 把 Task<T> move 进 TaskBase 的 shared_ptr，供调度器类型擦除持有
     */
    template <typename T>
    struct TaskModel : TaskBase
    {
        Task<T> task;
        explicit TaskModel(Task<T> t) : task(std::move(t)) {}

        void resume() override { task.resume(); }
        bool done() const noexcept override { return task.done(); }

        // 绑定回调函数到 协程句柄内部的 on_complete 中
        void setOnComplete(std::function<void()> cb) override
        {
            auto h = task.getHandle();
            if (h)
            {
                h.promise().on_complete = std::move(cb);
            }
        }

        // 获取是否进入了 final_suspend 函数
        bool finalizing() const noexcept override
        {
            auto h = task.getHandle();
            return h && h.promise().finalizing.load(std::memory_order_acquire);
        }
    };

    /**
     * @brief 把 Task<T> 转换为 shared_ptr<TaskBase>
     */
    template <typename T>
    inline std::shared_ptr<TaskBase> makeTaskBase(Task<T> task)
    {
        return std::make_shared<TaskModel<T>>(std::move(task));
    }

} // namespace blue