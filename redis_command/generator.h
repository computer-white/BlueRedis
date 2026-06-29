#pragma once
#include <coroutine>
#include <exception>
#include <utility>
#include <iterator>

namespace blue
{
    template <typename T>
    struct Generator
    {
        struct promise_type
        {
            T val;
            std::exception_ptr exception;
            Generator get_return_object()
            {
                return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            std::suspend_always yield_value(T v)
            {
                val = std::move(v);
                return {};
            }

            void return_void() { return; }
            void unhandled_exception()
            {
                exception = std::current_exception();
                return;
            }
            ~promise_type() = default;
        };

    public:
        using HandleType = std::coroutine_handle<promise_type>;
        using value_type = T;

        Generator() : handle(nullptr) {}
        explicit Generator(HandleType h) : handle(h) {}

        Generator(const Generator &) = delete;
        Generator &operator=(const Generator &) = delete;

        Generator(Generator &&other) noexcept
            : handle(std::exchange(other.handle, nullptr)) {}

        Generator &operator=(Generator &&other) noexcept
        {
            if (this != &other)
            {
                if (handle)
                {
                    handle.destroy();
                    handle = nullptr;
                }
                handle = std::exchange(other.handle, nullptr);
            }
            return *this;
        }

        ~Generator()
        {
            if (handle)
            {
                handle.destroy();
                handle = nullptr;
            }
        }

        struct Iterator
        {
            HandleType handle;

            bool operator!=(std::default_sentinel_t) const
            {
                if (!handle || handle.done())
                {
                    return false;
                }
                if (handle.promise().exception)
                {
                    std::rethrow_exception(handle.promise().exception);
                }
                return true;
            }

            Iterator &operator++()
            {
                handle.resume();
                if (handle.promise().exception)
                {
                    std::rethrow_exception(handle.promise().exception);
                }
                return *this;
            }

            T operator*() const
            {
                if (handle.promise().exception)
                {
                    std::rethrow_exception(handle.promise().exception);
                }
                return handle.promise().val;
            }
        };

        Iterator begin()
        {
            if (!handle || handle.done())
            {
                return {nullptr};
            }
            handle.resume();
            return {handle};
        }

        std::default_sentinel_t end() { return {}; }

        T &value() const
        {
            if (handle.promise().exception)
            {
                std::rethrow_exception(handle.promise().exception);
            }
            return handle.promise().val;
        }

        bool done() const
        {
            return !handle || handle.done();
        }

        void resume()
        {
            if (handle.promise().exception)
            {
                std::rethrow_exception(handle.promise().exception);
            }
            if (handle && !handle.done())
            {
                handle.resume();
            }
        }

    private:
        HandleType handle;
    };

    // 管道节点
    template <typename Source, typename Operation>
    class Pipeline
    {
    public:
        Pipeline(Source s, Operation o) : source(std::move(s)), op(std::move(o)) {}

        template <typename NextOp>
        auto operator|(NextOp &&next) &&
        {
            auto combined = [this_op = std::move(op), next = std::forward<NextOp>(next)]<typename S>(S &&src) mutable
            {
                auto result = this_op(std::forward<S>(src));
                return next(std::move(result));
            };
            return Pipeline<Source, decltype(combined)>(std::move(source), std::move(combined));
        }

        // 隐式类型转换
        operator Generator<typename Source::value_type>() &&
        {
            return op(std::move(source));
        }

        auto begin()
        {
            gen = std::move(*this).operator Generator<typename Source::value_type>();
            return gen.begin();
        }

        auto end()
        {
            return std::default_sentinel;
        }

    private:
        Source source;
        Operation op;
        Generator<typename Source::value_type> gen;
    };

    template <typename Container>
    auto from(Container &&cont)
    {
        using ValueType = std::remove_reference_t<Container>;
        using ItemType = typename ValueType::value_type;

        return Pipeline(std::forward<Container>(cont),
                        [](auto &&c) -> Generator<ItemType>
                        {
                            for (auto &&item : c)
                            {
                                co_yield std::forward<decltype(item)>(item);
                            }
                        });
    }

    template <typename Pred>
    auto filter(Pred &&pred)
    {
        // 返回下一个操作
        return [pred = std::forward<Pred>(pred)]<typename S>(S source) -> Generator<typename S::value_type>
        {
            for (auto &&val : source)
            {
                co_yield std::forward<decltype(val)>(val);
            }
        };
    }

    template <typename Func>
    auto transform(Func &&func)
    {
        return [func = std::forward<Func>(func)]<typename S>(S source) -> Generator<decltype(func(std::declval<typename S::value_type>()))>
        {
            for (auto &&val : source)
            {
                co_yield func(val);
            }
        };
    }

    template <typename T>
    auto take(T count)
    {
        return [count]<typename S>(S source) -> Generator<typename S::value_type>
        {
            T i = T();
            for (auto &&val : source)
            {
                if (i >= count)
                {
                    break;
                }
                co_yield std::forward<decltype(val)>(val);
                i++;
            }
        };
    }
}