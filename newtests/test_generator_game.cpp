#include <iostream>
#include <numeric>
#include <vector>
#include <list>
#include <unordered_map>
#include "redis_command/generator.h"

using namespace blue;

void test_vector()
{
    std::vector<int> vec(100,0);
    std::iota(vec.begin(), vec.end(), 0);
    auto num = blue::from(vec) | 
            blue::filter([](int x) { return (x & 1) == 0; }) |
            blue::transform([](int x) { return x * 2; }) |
            blue::take(20) | 
            blue::drop(10) | 
            blue::take_while([](int x) { return x < 60; });
    for (const auto& x : num)
    {
        std::cout << x << " ";
    }
    std::cout << "\n";
}

void test_list()
{
    std::list<int> lis = {1,2,3,4,5,6};
    auto num = blue::from(lis) | 
                blue::filter([](int x) { return (x & 1) == 0; }) |
                blue::transform([](int x) { return x * 10; }) |
                blue::drop_while([](int x) { return x < 50; });
    for (const auto& x : num)
    {
        std::cout << x << " ";
    }
    std::cout << "\n";
}

void test_umap()
{
    std::unordered_map<int, int> map = {
        {1, 2},
        {4, 3},
        {5, 6},
        {8, 7}
    };

    auto m = blue::from(map) | 
            blue::filter([](auto tem) { return (tem.first & 1) == 0; });
    for (const auto & [key, val] : m)
    {
        std::cout << key << ":" << val << " ";
    }
    std::cout << "\n";
}

void test_distinct()
{
    std::vector<int> vec = {1,1,1,2,2,3,4,4,5,7,8};
    auto num = blue::from(vec) | 
                blue::distinct();
    for (const auto& x : num)
    {
        std::cout << x << " ";
    }
    std::cout << "\n";
}

void test_reduce()
{
    std::vector<int> vec = {1,2,3,4,5};
    auto num = blue::from(vec) |
                blue::reduce(0, [](int tem, int x) { return tem + x; });
    std::cout << (int)num << "\n";
}

void test_scan()
{
    std::vector<int> vec = {1,2,3,4,5};
    auto num = blue::from(vec) |
                blue::scan(0, [](int tem, int x) { return tem + x; });
    for (const auto& x : num)
    {
        std::cout << x << " ";
    }
    std::cout << "\n";
}

void test_count()
{
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 测试1：全部计数
    size_t cnt1 = blue::from(vec) | blue::count();
    std::cout << "Total: " << cnt1 << "\n";  // 10
    
    // 测试2：过滤后计数
    size_t cnt2 = blue::from(vec) | 
                blue::filter([](int x) { return x % 2 == 0; }) |
                blue::count();
    std::cout << "Even: " << cnt2 << "\n";  // 5
    
    // 测试3：take 后计数
    size_t cnt3 = blue::from(vec) | 
                blue::take(3) |
                blue::count();
    std::cout << "First 3: " << cnt3 << "\n";  // 3
    
    // 测试4：空流
    std::vector<int> empty;
    size_t cnt4 = blue::from(empty) | blue::count();
    std::cout << "Empty: " << cnt4 << "\n";  // 0
}

void test_to_vector()
{
    auto result = blue::from({1, 2, 3, 4, 5}) | 
              blue::filter([](int x) { return x % 2 == 0; }) |
              blue::to_vector();
    for (const auto& x : result.eval())
    {
        std::cout << x << " ";
    }
    std::cout << "\n";
}

void test_to_list()
{
    auto result = blue::from({1,2,2,2,3,4,5,5,5,6,6,7,8}) |
                    blue::distinct() |
                    blue::to_list();
    for (const auto& x : result.eval())
    {
        std::cout << x << " ";
    }
    std::cout << "\n";
}

void test_to_unordered_map()
{
    std::vector<std::pair<int, std::string>> data = {{1, "a"}, {2, "b"}, {3, "c"}};
    auto result = blue::from(data) |
                    blue::to_unordered_map(
                        [](const std::pair<int, std::string>& p) { return p.first; },
                        [](const std::pair<int, std::string>& p) { return p.second; });
    for (const auto& [key, val] : result.eval())
    {
        std::cout << key << ":" << val << " ";
    }
    std::cout << "\n";
}

void test_to_map()
{
    std::vector<std::pair<int, std::string>> data = {{1, "a"}, {2, "b"}, {3, "c"}};
    auto result = blue::from(data) |
                    blue::to_map([](const std::pair<int, std::string>& p) { return p.first; },
                                    [](const std::pair<int, std::string>& p) { return p.second; });
    for (const auto& [key, val] : result.eval())
    {
        std::cout << key << ":" << val << " ";
    }
    std::cout << "\n";
}

int main()
{
    test_vector();
    test_list();
    test_umap();
    test_distinct();
    test_reduce();
    test_scan();
    test_count();
    test_to_vector();
    test_to_list();
    test_to_unordered_map();
    test_to_map();
}