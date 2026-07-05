/**
 * @file generator.h
 * @brief 协程实现的异步生成器模块,支持管道运算
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.28
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <coroutine>
#include <exception>
#include <utility>
#include <optional>
#include <list>
#include <vector>
#include <unordered_map>
#include <map>
#include <functional>
#include <type_traits>
#include <iterator>

namespace blue
{
    template <typename T>
    struct Generator
    {
        struct promise_type
        {
            std::optional<T> val;
            std::exception_ptr exception;
            Generator get_return_object()
            {
                return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            std::suspend_always yield_value(T v)
            {
                val.emplace(std::move(v));
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
                return *handle.promise().val;
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

        T eval() const
        {
            if (handle.promise().exception)
            {
                std::rethrow_exception(handle.promise().exception);
            }
            return *handle.promise().val;
        }

        T &value() const
        {
            if (handle.promise().exception)
            {
                std::rethrow_exception(handle.promise().exception);
            }
            return *handle.promise().val;
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

    template <typename T>
    struct Terminal
    {
        T value;

        template <typename U>
        explicit Terminal(U &&u) : value(std::forward<U>(u)) {}
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
            // 去掉了 mutable
            auto combined = [this_op = std::move(op), next = std::forward<NextOp>(next)]<typename S>(S &&src)
            {
                auto result = this_op(std::forward<S>(src));
                return next(std::move(result));
            };
            return Pipeline<Source, decltype(combined)>(std::move(source), std::move(combined));
        }

        // 可以显示输出为最终类型 eg: std::list<int> res = blue::from({1,2,3,3}) | blue::to_list();
        template <typename T>
        operator T() const
        {
            auto result = op(source);
            if constexpr (requires { result.value; })
            {
                return result.value;
            }
            else
            {
                return result;
            }
        }

        // 输出为最终类型 eg: auto res = blue::from({1,2,3}) | blue::to_list(); for (const auto& x : res.eval()) {...}
        auto eval() const
        {
            auto result = op(source);
            if constexpr (requires { result.value; })
            {
                return result.value;
            }
            else
            {
                return result;
            }
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

    template <typename T>
    concept Container = requires(T t) {
        typename std::decay_t<T>::value_type;
        t.begin();
        t.end();
    };

    template <typename T>
    concept InitializerList = requires(T t) {
        typename std::decay_t<T>::value_type;
        std::is_same_v<std::decay_t<T>, std::initializer_list<typename std::decay_t<T>::value_type>>;
    };

    template <typename T>
    concept GeneratorType = requires(T t) {
        typename std::decay_t<T>::value_type;
        t.begin();
        t.end();
        t.done();
        t.resume();
    };

    // 数据来源
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

    template <typename T>
    auto from(std::initializer_list<T> list)
    {
        return Pipeline(list,
                        [](auto &&c) -> Generator<T>
                        {
                            for (const auto &item : c)
                            {
                                co_yield item;
                            }
                        });
    }

    // 过滤
    template <typename Pred>
    auto filter(Pred &&pred)
    {
        // 返回下一个操作
        return [pred = std::forward<Pred>(pred)]<typename S>(S source) -> Generator<typename S::value_type>
        {
            for (auto &&val : source)
            {
                if (pred(val))
                {
                    co_yield std::forward<decltype(val)>(val);
                }
            }
        };
    }

    // 将值转化为func(x)
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

    // 取前count个
    template <typename T = int>
    auto take(T count)
    {
        return [count]<typename S>(S source) -> Generator<typename S::value_type>
        {
            int i = 0;
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

    // 跳过前count 个
    template <typename T = int>
    auto drop(T count)
    {
        return [count]<typename S>(S source) -> Generator<typename S::value_type>
        {
            int i = 0;
            for (auto &&val : source)
            {
                if (i < count)
                {
                    i++;
                    continue;
                }
                co_yield std::forward<decltype(val)>(val);
            }
        };
    }

    // 按条件取元素
    template <typename Pred>
    auto take_while(Pred pred)
    {
        return [pred = std::forward<Pred>(pred)]<typename S>(S source) -> Generator<typename S::value_type>
        {
            for (auto &&val : source)
            {
                if (pred(val))
                {
                    co_yield std::forward<decltype(val)>(val);
                }
            }
        };
    }

    // 按条件跳过元素
    template <typename Pred>
    auto drop_while(Pred pred)
    {
        return [pred = std::forward<Pred>(pred)]<typename S>(S source) -> Generator<typename S::value_type>
        {
            for (auto &&val : source)
            {
                if (pred(val))
                {
                    continue;
                }
                co_yield std::forward<decltype(val)>(val);
            }
        };
    }

    // 连续去重
    auto distinct()
    {
        return []<typename S>(S source) -> Generator<typename S::value_type>
        {
            int i = 0;
            using type = typename S::value_type;
            type prev = type();
            for (auto &&val : source)
            {
                if (i++ == 0)
                {
                    prev = val;
                    co_yield std::forward<decltype(val)>(val);
                    continue;
                }
                if (prev == val)
                {
                    continue;
                }
                prev = val;
                co_yield std::forward<decltype(val)>(val);
            }
        };
    }

    // 归约
    template <typename T = int, typename Func>
    auto reduce(T init, Func func)
    {
        return [init = std::move(init), func = std::forward<Func>(func)]<typename S>(S source)
        {
            using ResultType = std::invoke_result_t<Func, T, typename S::value_type>;
            ResultType res = init;
            for (auto &&val : source)
            {
                res = func(std::move(res), std::forward<decltype(val)>(val));
            }
            return Terminal<ResultType>{std::move(res)};
        };
    }

    // scan
    template <typename T = int, typename Func>
    auto scan(T init, Func func)
    {
        return [init = std::move(init), func = std::forward<Func>(func)]<typename S>(S source)
                   -> Generator<std::invoke_result_t<Func, T, typename S::value_type>>
        {
            using ResultType = std::invoke_result_t<Func, T, typename S::value_type>;
            ResultType res = init;
            for (auto &&val : source)
            {
                res = func(std::move(res), std::forward<decltype(val)>(val));
                co_yield res;
            }
        };
    }

    // count
    template <typename T = size_t>
    auto count()
    {
        return []<typename S>(S source)
        {
            T res = 0;
            for (auto &&_ : source)
            {
                res++;
            }
            return Terminal<T>{std::move(res)};
        };
    }

    // 输出为vector
    auto to_vector()
    {
        return []<typename S>(S source)
        {
            using ValueType = typename S::value_type;
            std::vector<ValueType> result;
            for (auto &&val : source)
            {
                result.emplace_back(std::forward<decltype(val)>(val));
            }
            return result;
        };
    }

    // 输出为list
    auto to_list()
    {
        return []<typename S>(S source)
        {
            using ValueType = typename S::value_type;
            std::list<ValueType> result;
            for (auto &&val : source)
            {
                result.emplace_back(std::forward<decltype(val)>(val));
            }
            return result;
        };
    }

    // 输出为unordered_map
    template <typename KeyFunc, typename ValueFunc>
    auto to_unordered_map(KeyFunc keyf, ValueFunc valuef)
    {
        return [keyfunc = std::forward<KeyFunc>(keyf),
                valfunc = std::forward<ValueFunc>(valuef)]<typename S>(S &&source)
        {
            using ElementType = std::decay_t<decltype(*std::begin(source))>;
            using KeyType = std::decay_t<decltype(keyfunc(std::declval<ElementType>()))>;
            using ValueType = std::decay_t<decltype(valfunc(std::declval<ElementType>()))>;

            std::unordered_map<KeyType, ValueType> result;

            for (auto &&val : source)
            {
                auto key = keyfunc(val);
                result.emplace(std::move(key), valfunc(std::forward<decltype(val)>(val)));
            }
            return result;
        };
    }

    // 支持自定义哈希和比较器
    template <typename KeyFunc, typename ValueFunc, typename Hash, typename KeyEqual = std::equal_to<>>
    auto to_unordered_map(KeyFunc keyf, ValueFunc valuef, Hash hash, KeyEqual equal = {})
    {
        return [keyfunc = std::forward<KeyFunc>(keyf),
                valfunc = std::forward<ValueFunc>(valuef),
                hash = std::move(hash),
                equal = std::move(equal)]<typename S>(S &&source)
        {
            using ElementType = std::decay_t<decltype(*std::begin(source))>;
            using KeyType = std::decay_t<decltype(keyfunc(std::declval<ElementType>()))>;
            using ValueType = std::decay_t<decltype(valfunc(std::declval<ElementType>()))>;

            std::unordered_map<KeyType, ValueType,
                               std::decay_t<Hash>,
                               std::decay_t<KeyEqual>>
                result(0, hash, equal);

            for (auto &&val : source)
            {
                auto key = keyfunc(val);
                result.emplace(std::move(key), valfunc(std::forward<decltype(val)>(val)));
            }
            return result;
        };
    }

    // 输出为map
    template <typename KeyFunc, typename ValueFunc, typename Compare = std::less<>>
    auto to_map(KeyFunc keyfunc, ValueFunc valuef, Compare com = {})
    {
        return [keyfunc = std::forward<KeyFunc>(keyfunc),
                valfunc = std::forward<ValueFunc>(valuef),
                com = std::move(com)]<typename S>(S &&source)
        {
            using KeyType = std::decay_t<decltype(keyfunc(std::declval<typename S::value_type>()))>;
            using ValueType = std::decay_t<decltype(valfunc(std::declval<typename S::value_type>()))>;

            // 类型转换
            using CompareType = std::conditional_t<
                std::is_same_v<std::decay_t<Compare>, std::less<>>,
                std::less<KeyType>,
                std::decay_t<Compare>>;

            // 创建比较器
            CompareType comparator = [&]()
            {
                if constexpr (std::is_same_v<std::decay_t<Compare>, std::less<>>)
                {
                    return std::less<KeyType>{};
                }
                else
                {
                    return com;
                }
            }();

            std::map<KeyType, ValueType, CompareType> result(comparator);

            for (auto &&val : source)
            {
                result.emplace(keyfunc(val), valfunc(std::forward<decltype(val)>(val)));
            }
            return result;
        };
    }
}