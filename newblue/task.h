#pragma once
#include <coroutine>
#include <exception>
#include <utility>

namespace newblue
{
    struct Task;
    struct SubCorroutine
    {
        std::coroutine_handle<> fa;
        bool await_ready() const noexcept { return false; }
        // call 永远是当前挂起的协程
        void await_suspend(std::coroutine_handle<>) const noexcept
        {
            if (fa)
            {
                fa.resume();
            }
        }
        void await_resume() const noexcept {}
        SubCorroutine(std::coroutine_handle<> &p) : fa(p) {}
    };
    struct Task
    {
        struct promise_type
        {
            std::coroutine_handle<> fa;
            std::exception_ptr exception;
            Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
            std::suspend_always initial_suspend() { return {}; }
            SubCorroutine final_suspend() noexcept { return SubCorroutine{fa}; }
            void return_void() {}
            void unhandled_exception()
            {
                exception = std::current_exception();
            }
        };
        using HandleType = std::coroutine_handle<promise_type>;
        Task() : handle(nullptr) {}
        explicit Task(HandleType h) : handle(std::exchange(h, nullptr)) {}
        ~Task()
        {
            if (handle)
            {
                handle.destroy();
            }
        }

        Task(const Task &) = delete;
        Task &operator=(const Task &) = delete;

        Task(Task &&other) : handle(std::exchange(other.handle, nullptr)) {}
        Task &operator=(Task &&other)
        {
            if (this != &other)
            {
                if (handle)
                {
                    handle.destroy();
                }
                handle = std::exchange(other.handle, nullptr);
            }
            return *this;
        }

        bool done() const
        {
            return handle.done();
        }

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

        operator bool() const { return handle != nullptr; }

        Task& operator=(std::nullptr_t) 
        {
            if (handle) handle.destroy();
            handle = nullptr;
            return *this;
        }

        bool await_ready() const noexcept
        { // 等待(true)挂起(false)，只有在挂起后才会调用await_suspend
            return handle && handle.done();
        }

        void await_suspend(std::coroutine_handle<> call) const noexcept
        {
            if (handle && !handle.done())
            {
                handle.promise().fa = std::move(call);
                handle.resume();
            }
        }

        void await_resume() const noexcept
        { // 挂起后被恢复调用这个拿值
            if (handle.promise().exception)
            {
                std::rethrow_exception(handle.promise().exception);
            }
        }

    private:
        HandleType handle;
    };
}
