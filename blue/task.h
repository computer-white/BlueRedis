#pragma once
#include <coroutine>
#include <exception>
#include <utility>

namespace blue
{
    template <typename T = void>
    struct Task;

    struct SubCorroutine
    {
        std::coroutine_handle<> fa;
        
        bool await_ready() const noexcept { return false; }
        
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) const noexcept
        {
            if (fa && fa.address())
            {
                return fa;
            }
            return std::noop_coroutine();
        }
        
        void await_resume() const noexcept {}
        
        explicit SubCorroutine(std::coroutine_handle<> p = nullptr) : fa(p) {}
    };

    template <typename T>
    struct Task
    {
        struct promise_type
        {
            std::coroutine_handle<> fa;
            std::exception_ptr exception;
            T val;
            
            Task get_return_object() 
            { 
                return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; 
            }
            
            std::suspend_always initial_suspend() { return {}; }
            
            SubCorroutine final_suspend() noexcept 
            { 
                return SubCorroutine{fa}; 
            }
            
            void return_value(T v) { val = std::move(v); }

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
        
        T get() const
        {
            if (handle && handle.promise().exception)
            {
                std::rethrow_exception(handle.promise().exception);
            }
            return std::move(handle.promise().val);
        }

        HandleType getHandle() noexcept 
        { 
            return std::exchange(handle, nullptr); 
        }

        void destroy()
        {
            destroySafe();
        }

        explicit operator bool() const noexcept { return handle != nullptr; }

        Task& operator=(std::nullptr_t) noexcept
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

    // Task<void> 特化
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
            
            void return_void() {}
            
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

        HandleType getHandle() noexcept 
        { 
            return std::exchange(handle, nullptr); 
        }

        void destroy()
        {
            destroySafe();
        }

        explicit operator bool() const noexcept { return handle != nullptr; }

        Task& operator=(std::nullptr_t) noexcept
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
}