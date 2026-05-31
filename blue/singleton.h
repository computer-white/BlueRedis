#ifndef BLUE_SINGLETON_H
#define BLUE_SINGLETON_H
#include <memory>

// 单例模式模块
namespace blue
{
    /*
        X : 代表不同类型的单例
        N : 代表版本号
    */

    // 不带参数单例返回裸指针
    template <class T, class X = void, int N = 0>
    class SingleTon
    {
    public:
        /**
         * @brief 不带参数单例返回裸指针
         * @return T*
         */
        static T *GetInstance()
        {
            static T t;
            return &t;
        }
        SingleTon(const SingleTon &) = delete;
        SingleTon &operator=(const SingleTon &) = delete;
    };

    // 不带参数单例返回智能指针
    template <class T, class X = void, int N = 0>
    class SingleTonPtr
    {
    public:
        /**
         * @brief 不带参数单例返回智能指针
         * @return std::shared_ptr<T>
         */
        static std::shared_ptr<T> GetInstance()
        {
            static std::shared_ptr<T> v = std::make_shared<T>();
            return v;
        }
        SingleTonPtr(const SingleTonPtr &) = delete;
        SingleTonPtr &operator=(const SingleTonPtr &) = delete;
    };

    /*
        支持带参数的单例模式
    */

    // 带参数单例返回裸指针
    template <class T, class X = void, int N = 0>
    class SingleTonWithPa
    {
    public:
        /**
         * @brief 带参数单例返回裸指针
         * @return T*
         */
        template <typename... Args>
        static T *GetInstance(Args &&...args)
        {
            static T t(std::forward<Args>(args)...);
            return &t;
        }
        SingleTonWithPa(const SingleTonWithPa &) = delete;
        SingleTonWithPa &operator=(const SingleTonWithPa &) = delete;
    };

    // 带参数单例返回智能指针
    template <class T, class X = void, int N = 0>
    class SingleTonPtrWithPa
    {
    public:
        /**
         * @brief 带参数单例返回智能指针
         * @return std::shared_ptr<T>
         */
        template <typename... Args>
        static std::shared_ptr<T> GetInstance(Args &&...args)
        {
            static std::shared_ptr<T> v = std::make_shared<T>(std::forward<Args>(args)...);
            return v;
        }
        SingleTonPtrWithPa(const SingleTonPtrWithPa &) = delete;
        SingleTonPtrWithPa &operator=(const SingleTonPtrWithPa &) = delete;
    };

}
#endif // __BLUE_SINGLETON_H__