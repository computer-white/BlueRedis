/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <gtest/gtest.h>
#include <iostream>
#include <numeric>
#include <vector>
#include <string>
#include <functional>
#include <type_traits>
#include "redis_command/generator.h"

namespace
{
    // 1. 测试简单的 Generator
    blue::Generator<int> simpleGenerator()
    {
        for (int i = 0; i < 5; ++i)
        {
            co_yield i;
        }
    }

    // 2. 测试带参数的 Generator
    blue::Generator<std::string> stringGenerator(const std::vector<std::string> &words)
    {
        for (const auto &word : words)
        {
            co_yield word;
        }
    }

    // 3. 测试 Pipeline + Generator
    blue::Generator<int> filterEven(blue::Generator<int> source)
    {
        for (auto val : source)
        {
            if (val % 2 == 0)
            {
                co_yield val;
            }
        }
    }

    blue::Generator<int> multiplyBy2(blue::Generator<int> source)
    {
        for (auto val : source)
        {
            co_yield val * 2;
        }
    }

    // 4. 测试 from + pipeline
    blue::Generator<int> testPipeline()
    {
        std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        auto result = blue::from(data) |
                      blue::filter([](int x)
                                   { return x % 2 == 0; }) |
                      blue::transform([](int x)
                                      { return x * 2; }) |
                      blue::take(3);

        for (auto val : result)
        {
            co_yield val;
        }
    }

    template <typename T>
    struct is_vector : std::false_type
    {
    };

    template <typename T, typename Alloc>
    struct is_vector<std::vector<T, Alloc>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_vector_v = is_vector<T>::value;

    template <typename T>
    struct is_list : std::false_type
    {
    };

    template <typename T, typename Alloc>
    struct is_list<std::list<T, Alloc>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_list_v = is_list<T>::value;

    template <typename T>
    struct is_umap : std::false_type
    {
    };

    template <typename Key, typename Tp,
              typename Hash, typename Pred,
              typename Alloc>
    struct is_umap<std::unordered_map<Key, Tp, Hash, Pred, Alloc>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_umap_v = is_umap<T>::value;
}

TEST(GeneratorTest, SimpleGeneratorTest)
{
    std::vector<int> vec;
    auto gen = simpleGenerator();
    for (auto val : gen)
    {
        vec.push_back(val);
    }
    EXPECT_EQ(vec, (std::vector<int>{0, 1, 2, 3, 4}));
}

TEST(GeneratorTest, StringGeneratorTest)
{
    std::vector<std::string> words = {"hello", "world", "from", "generator"};
    std::vector<std::string> tem;
    tem.reserve(words.size());
    auto gen = stringGenerator(words);
    for (const auto &word : gen)
    {
        tem.push_back(word);
    }
    EXPECT_EQ(tem, words);
}

TEST(GeneratorTest, FilterAndTransform)
{
    auto gen3 = simpleGenerator();
    auto filtered = filterEven(std::move(gen3));
    auto transformed = multiplyBy2(std::move(filtered));
    std::vector<int> res;
    for (auto val : transformed)
    {
        // 0 4 8
        res.push_back(val);
    }
    EXPECT_EQ(res, (std::vector<int>{0, 4, 8}));
}

TEST(GeneratorTest, PipelineTest)
{
    auto gen4 = testPipeline();
    std::vector<int> res;
    for (auto val : gen4)
    {
        // 2 4 6 -> 4 8 12
        res.push_back(val);
    }
    EXPECT_EQ(res, (std::vector<int>{4, 8, 12}));
}

TEST(GeneratorTest, MultiIterate)
{
    std::vector<int> res;
    auto gen = simpleGenerator();
    for (auto x : gen)
    {
        res.push_back(x);
    }
    EXPECT_EQ(res, (std::vector<int>{0, 1, 2, 3, 4}));
    res.clear();
    for (auto x : gen)
    {
        res.push_back(x);
    }
    EXPECT_TRUE(res.empty());
}

TEST(GeneratorTest, EmptyGenerator)
{
    std::vector<int> empty_data;
    auto gen6 = blue::from(empty_data);
    int count = 0;
    for (auto val : gen6)
    {
        count++;
    }
    EXPECT_EQ(count, 0);
}

TEST(GeneratorAdvancedTest, FromVector)
{
    std::vector<int> vec(100, 0);
    std::iota(vec.begin(), vec.end(), 0);
    auto num = blue::from(vec) |
               blue::filter([](int x)
                            { return (x & 1) == 0; }) |
               blue::transform([](int x)
                               { return x * 2; }) |
               blue::take(20) |
               blue::drop(10) |
               blue::take_while([](int x)
                                { return x < 60; });
    std::vector<int> res;
    for (const auto &x : num)
    {
        res.push_back(x);
    }
    EXPECT_TRUE(res.size() <= 10);
    EXPECT_EQ(res, (std::vector<int>{40, 44, 48, 52, 56}));
}

TEST(GeneratorAdvancedTest, FromList)
{
    std::list<int> lis = {1, 2, 3, 4, 5, 6};
    auto num = blue::from(lis) |
               blue::filter([](int x)
                            { return (x & 1) == 0; }) |
               blue::transform([](int x)
                               { return x * 10; }) |
               blue::drop_while([](int x)
                                { return x < 50; });
    std::vector<int> res;
    for (const auto &x : num)
    {
        res.push_back(x);
    }
    EXPECT_EQ(res.size(), 1);
    EXPECT_EQ(res, (std::vector<int>{60}));
}

TEST(GeneratorAdvancedTest, FromUMap)
{
    std::unordered_map<int, int> map = {
        {1, 2},
        {4, 3},
        {5, 6},
        {8, 7}};

    auto m = blue::from(map) |
             blue::filter([](auto tem)
                          { return (tem.first & 1) == 0; });
    std::vector<std::pair<int, int>> res;
    for (const auto &[key, val] : m)
    {
        res.emplace_back(key, val);
    }
    EXPECT_EQ(res.size(), 2);
}

TEST(GeneratorAdvancedTest, FromvecDistinct)
{
    std::vector<int> vec = {1, 1, 1, 2, 2, 3, 4, 4, 5, 7, 8};
    auto num = blue::from(vec) |
               blue::distinct();
    std::vector<int> res;
    for (const auto &x : num)
    {
        res.push_back(x);
    }
    EXPECT_TRUE(res.size() < vec.size());
    EXPECT_EQ(res, (std::vector<int>{1, 2, 3, 4, 5, 7, 8}));
}

TEST(GeneratorAdvancedTest, FromVecReduce)
{
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto num = blue::from(vec) |
               blue::reduce(0, [](int tem, int x)
                            { return tem + x; });
    int x = std::accumulate(vec.begin(), vec.end(), 0);
    EXPECT_EQ(x, num.eval());
}

TEST(GeneratorAdvancedTest, FromVecScan)
{
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto num = blue::from(vec) |
               blue::scan(0, [](int tem, int x)
                          { return tem + x; });
    auto num_vec = num.eval();
    int sum = 0, i = 0;
    for (const auto &x : num_vec)
    {
        sum += vec[i++];
        EXPECT_EQ(sum, x);
    }
}

TEST(GeneratorAdvancedTest, FromVectorCount)
{
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    size_t cnt = blue::from(vec) | blue::count();
    EXPECT_EQ(cnt, 10);

    cnt = blue::from(vec) |
          blue::filter([](int x)
                       { return x % 2 == 0; }) |
          blue::count();
    EXPECT_EQ(cnt, 5);

    cnt = blue::from(vec) |
          blue::take(3) |
          blue::count();
    EXPECT_EQ(cnt, 3);

    std::vector<int> empty;
    cnt = blue::from(empty) | blue::count();
    EXPECT_EQ(cnt, 0);
}

TEST(GeneratorAdvancedTest, ToVector)
{
    auto res = blue::from({1, 2, 3, 4, 5}) |
               blue::filter([](int x)
                            { return x % 2 == 0; }) |
               blue::to_vector();
    EXPECT_TRUE((is_vector_v<decltype(res.eval())>));
    std::vector<int> tem;
    for (const auto &x : res.eval())
    {
        tem.push_back(x);
    }
    EXPECT_EQ(tem, (std::vector<int>{2, 4}));
}

TEST(GeneratorAdvancedTest, ToList)
{
    auto result = blue::from({1, 2, 2, 2, 3, 4, 5, 5, 5, 6, 6, 7, 8}) |
                  blue::distinct() |
                  blue::to_list();
    EXPECT_TRUE((is_list_v<decltype(result.eval())>));
    std::list<int> tem;
    for (const auto &x : result.eval())
    {
        tem.push_back(x);
    }
    EXPECT_EQ(tem, (std::list<int>{1, 2, 3, 4, 5, 6, 7, 8}));
}

TEST(GeneratorAdvancedTest, ToUnorderedMap)
{
    std::vector<std::pair<int, std::string>> data = {{1, "a"}, {2, "b"}, {3, "c"}};
    auto result = blue::from(data) |
                  blue::to_unordered_map(
                      [](const std::pair<int, std::string> &p)
                      { return p.first; },
                      [](const std::pair<int, std::string> &p)
                      { return p.second; });
    auto map = result.eval();
    static_assert(std::is_same_v<
                  std::remove_cvref_t<decltype(map)>,
                  std::unordered_map<int, std::string>>);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map.at(1), "a");
    EXPECT_EQ(map.at(2), "b");
    EXPECT_EQ(map.at(3), "c");
}

TEST(GeneratorAdvancedTest, ToMap)
{
    std::vector<std::pair<int, std::string>> data = {{1, "a"}, {2, "b"}, {3, "c"}};
    auto result = blue::from(data) |
                  blue::to_map([](const std::pair<int, std::string> &p)
                               { return p.first; },
                               [](const std::pair<int, std::string> &p)
                               { return p.second; });
    auto map = result.eval();
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(map)>,
                                 std::map<int, std::string>>);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map.at(1), "a");
    EXPECT_EQ(map.at(2), "b");
    EXPECT_EQ(map.at(3), "c");
}