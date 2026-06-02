#pragma once
#include <coroutine>
#include <exception>
#include <utility>

namespace blue
{
    template <typename T = void>
    struct Task;

    /**
     * @brief 子协程 final_suspend 的返回类型，用于对称转移
     *
     * 当子协程执行完毕后，不回到调度器，而是直接跳转到父协程继续执行。
     * 减少了调度开销，调用栈更浅。
     */
    struct SubCorroutine
    {
        std::coroutine_handle<> fa; // 待恢复的父协程句柄

        /**
         * @brief 不提前返回，总是进入 await_suspend
         */
        bool await_ready() const noexcept { return false; }

        /**
         * @brief 对称转移：有父协程则直接跳转，否则挂起在 noop 上
         * @param caller 当前正在挂起的协程（子协程自身）
         * @return 下一个要恢复的协程句柄
         */
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) const noexcept
        {
            if (fa && fa.address())
            {
                return fa; // 直接跳转到父协程（对称转移）
            }
            return std::noop_coroutine(); // 没有父协程，挂起在空操作上
        }

        /**
         * @brief 恢复时无需额外操作
         */
        void await_resume() const noexcept {}

        /**
         * @brief 构造函数
         * @param p 父协程句柄（可为空）
         */
        explicit SubCorroutine(std::coroutine_handle<> p = nullptr) : fa(p) {}
    };

    /**
     * @brief 协程任务模板（有返回值）
     * @tparam T 协程返回值类型
     */
    template <typename T>
    struct Task
    {
        /**
         * @brief 协程要求的 promise_type，控制协程生命周期
         */
        struct promise_type
        {
            std::coroutine_handle<> fa;   // 父协程句柄(用于等待协程)
            std::exception_ptr exception; // 协程内部异常
            T val;                        // 协程返回值

            // 都是协程必要函数
            /**
             * @brief 协程创建时调用，返回 Task 对象
             */
            Task get_return_object()
            {
                return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            /**
             * @brief 协程创建后先挂起，由调度器决定何时启动
             */
            std::suspend_always initial_suspend() { return {}; }

            /**
             * @brief 协程结束时挂起，通过 SubCorroutine 对称转移回父协程
             */
            SubCorroutine final_suspend() noexcept
            {
                return SubCorroutine{fa};
            }

            /**
             * @brief co_return value 时调用
             */
            void return_value(T v) { val = std::move(v); }

            /**
             * @brief 协程内部抛出未捕获异常时调用
             */
            void unhandled_exception() { exception = std::current_exception(); }

            ~promise_type() = default;
        };

        using HandleType = std::coroutine_handle<promise_type>;

        // ==================== 构造 / 析构 ====================

        Task() : handle(nullptr) {}
        explicit Task(HandleType h) : handle(h) {}

        ~Task() = default;

        // 禁止拷贝
        Task(const Task &) = delete;
        Task &operator=(const Task &) = delete;

        // 允许移动
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

        /**
         * @brief 协程是否已完成
         */
        bool done() const noexcept { return !handle || handle.done(); }

        /**
         * @brief 手动恢复协程执行（调度器调用）
         * @throws 如果协程内部有未捕获异常，重新抛出
         */
        void resume() const
        {
            if (handle && !handle.done())
            {
                if (handle.promise().exception)
                {
                    std::rethrow_exception(handle.promise().exception);
                }
                handle.resume();
            }
        }

        /**
         * @brief 获取协程返回值（只能在协程完成后调用）
         * @throws 如果协程内部有未捕获异常，重新抛出
         */
        T get() const
        {
            if (handle && handle.promise().exception)
            {
                std::rethrow_exception(handle.promise().exception);
            }
            return std::move(handle.promise().val);
        }

        /**
         * @brief 获取类型化句柄（不转移所有权）
         */
        HandleType getHandle() noexcept { return handle; }

        /**
         * @brief 获取无类型句柄（用于跨类型传递）
         */
        std::coroutine_handle<> getHandleNoType() noexcept { return handle; }

        /**
         * @brief 立即销毁协程帧（谨慎使用，需确保协程已完成）
         */
        void destroy() { destroySafe(); }

        /**
         * @brief 判断 Task 是否有效（持有协程句柄）
         */
        explicit operator bool() const noexcept { return handle != nullptr; }

        /**
         * @brief 清空 Task（若协程已完成则销毁帧）
         */
        Task &operator=(std::nullptr_t) noexcept
        {
            destroySafe();
            handle = nullptr;
            return *this;
        }

        /**
         * @brief 若协程已完成，不挂起
         */
        bool await_ready() const noexcept { return !handle || handle.done(); }

        /**
         * @brief 挂起当前协程，设置父协程关系，启动子协程
         * @param call 父协程句柄
         */
        void await_suspend(std::coroutine_handle<> call) const noexcept
        {
            if (handle && !handle.done())
            {
                handle.promise().fa = call; // 记录父协程
                handle.resume();            // 启动子协程
            }
        }

        /**
         * @brief 子协程完成后获取返回值
         * @throws 如果子协程内部有异常，重新抛出
         */
        T await_resume() const
        {
            if (handle && handle.promise().exception)
            {
                std::rethrow_exception(handle.promise().exception);
            }
            return std::move(handle.promise().val);
        }

    private:
        HandleType handle;

        /**
         * @brief 安全销毁协程帧
         */
        void destroySafe()
        {
            if (handle)
            {
                if (handle.done())
                {
                    HandleType h = handle;
                    handle = nullptr;
                    h.destroy();
                }
                else
                {
                    handle = nullptr; // 协程未完成，仅释放引用
                }
            }
        }
    };

    // ==================== Task<void> 特化（无返回值） ====================
    template <>
    struct Task<void>
    {
        struct promise_type
        {
            std::coroutine_handle<> fa;
            std::exception_ptr exception;

            Task get_return_object()
            {
                return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() { return {}; }

            SubCorroutine final_suspend() noexcept
            {
                return SubCorroutine{fa};
            }

            void return_void() {} // co_return 无值

            void unhandled_exception() { exception = std::current_exception(); }

            ~promise_type() = default;
        };

        using HandleType = std::coroutine_handle<promise_type>;

        Task() : handle(nullptr) {}
        explicit Task(HandleType h) : handle(h) {}

        ~Task() = default;

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
                if (handle.promise().exception)
                {
                    std::rethrow_exception(handle.promise().exception);
                }
                handle.resume();
            }
        }

        HandleType getHandle() noexcept { return handle; }

        std::coroutine_handle<> getHandleNoType() noexcept { return handle; }

        void destroy() { destroySafe(); }

        explicit operator bool() const noexcept { return handle != nullptr; }

        Task &operator=(std::nullptr_t) noexcept
        {
            destroySafe();
            handle = nullptr;
            return *this;
        }

        bool await_ready() const noexcept { return !handle || handle.done(); }

        void await_suspend(std::coroutine_handle<> call) const noexcept
        {
            if (handle && !handle.done())
            {
                handle.promise().fa = call;
                handle.resume();
            }
        }

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
            if (handle)
            {
                if (handle.done())
                {
                    HandleType h = handle;
                    handle = nullptr;
                    h.destroy();
                }
                else
                {
                    handle = nullptr;
                }
            }
        }
    };

} // namespace blue