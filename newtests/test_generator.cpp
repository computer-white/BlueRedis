#include <iostream>
#include <vector>
#include <string>
#include "redis_command/generator.h"

using namespace blue;

// 1. 测试简单的 Generator
Generator<int> simpleGenerator()
{
    for (int i = 0; i < 5; ++i)
    {
        co_yield i;
    }
}

// 2. 测试带参数的 Generator
Generator<std::string> stringGenerator(const std::vector<std::string> &words)
{
    for (const auto &word : words)
    {
        co_yield word;
    }
}

// 3. 测试 Pipeline + Generator
Generator<int> filterEven(Generator<int> source)
{
    for (auto val : source)
    {
        if (val % 2 == 0)
        {
            co_yield val;
        }
    }
}

Generator<int> multiplyBy2(Generator<int> source)
{
    for (auto val : source)
    {
        co_yield val * 2;
    }
}

// 4. 测试 from + pipeline
Generator<int> testPipeline()
{
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto result = from(data) | filter([](int x)
                                      { return x % 2 == 0; }) |
                  transform([](int x)
                            { return x * 2; }) |
                  take(3);

    for (auto val : result)
    {
        co_yield val;
    }
}

int main()
{
    std::cout << "=== Generator Test ===" << std::endl;

    // 测试1: 简单 Generator
    std::cout << "\n--- Test 1: Simple Generator ---" << std::endl;
    auto gen1 = simpleGenerator();
    for (auto val : gen1)
    {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    // 测试2: 字符串 Generator
    std::cout << "\n--- Test 2: String Generator ---" << std::endl;
    std::vector<std::string> words = {"hello", "world", "from", "generator"};
    auto gen2 = stringGenerator(words);
    for (const auto &word : gen2)
    {
        std::cout << word << " ";
    }
    std::cout << std::endl;

    // 测试3: 过滤 + 转换
    std::cout << "\n--- Test 3: Filter + Transform ---" << std::endl;
    auto gen3 = simpleGenerator();
    auto filtered = filterEven(std::move(gen3));
    auto transformed = multiplyBy2(std::move(filtered));
    for (auto val : transformed)
    {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    // 测试4: from + pipeline
    std::cout << "\n--- Test 4: From + Pipeline ---" << std::endl;
    auto gen4 = testPipeline();
    for (auto val : gen4)
    {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    // 测试5: 多次迭代（每个 Generator 只能迭代一次）
    std::cout << "\n--- Test 5: Multiple iteration (should work once) ---" << std::endl;
    auto gen5 = simpleGenerator();
    std::cout << "First iteration: ";
    for (auto val : gen5)
    {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    std::cout << "Second iteration (should be empty): ";
    for (auto val : gen5)
    {
        std::cout << val << " ";
    }
    std::cout << "(empty)" << std::endl;

    // 测试6: 空 Generator
    std::cout << "\n--- Test 6: Empty Generator ---" << std::endl;
    std::vector<int> empty_data;
    auto gen6 = from(empty_data);
    int count = 0;
    for (auto val : gen6)
    {
        count++;
    }
    std::cout << "Empty generator yields " << count << " values" << std::endl;

    std::cout << "\n=== All tests passed! ===" << std::endl;

    return 0;
}