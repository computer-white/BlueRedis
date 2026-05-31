#ifndef BLUE_MTHREAD_H
#define BLUE_MTHREAD_H
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <functional>
#include <future>
#include <memory>
#include <atomic>

// 线程模块
namespace blue
{
    // 互斥锁模板
    template <typename T>
    class ScopedlockedImpl
    {
    public:
        ScopedlockedImpl(T &mutex) : m_mutex(mutex)
        {
            lock();
        }
        ~ScopedlockedImpl()
        {
            unlock();
        }

        /**
         * @brief 尝试获取锁
         * @return if successful, return 0
         */
        int try_lock()
        {
            int ret = m_mutex.try_lock();
            if (ret == 0)
            {
                m_locked = true;
            }
            return ret;
        }

        /**
         * @brief 获取锁
         * @return
         */
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.lock();
                m_locked = true;
            }
        }

        /**
         * @brief 解锁
         * @return
         */
        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    public:
        ScopedlockedImpl(const ScopedlockedImpl &) = delete;
        ScopedlockedImpl &operator=(const ScopedlockedImpl &) = delete;

    private:
        T &m_mutex;
        bool m_locked;
    };

    // 读锁模板
    template <typename T>
    class ReadScopedlockedImpl
    {
    public:
        ReadScopedlockedImpl(T &mutex) : m_mutex(mutex)
        {
            lock();
        }
        ~ReadScopedlockedImpl()
        {
            unlock();
        }

        /**
         * @brief 尝试获取锁
         * @return if successful, return 0
         */
        int try_lock()
        {
            int ret = m_mutex.try_lock_shared();
            if (ret == 0)
            {
                m_locked = true;
            }
            return ret;
        }

        /**
         * @brief 获取锁
         * @return
         */
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.lock_shared();
                m_locked = true;
            }
        }

        /**
         * @brief 解锁
         * @return
         */
        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock_shared();
                m_locked = false;
            }
        }

    public:
        ReadScopedlockedImpl(const ReadScopedlockedImpl &) = delete;
        ReadScopedlockedImpl &operator=(const ReadScopedlockedImpl &) = delete;

    private:
        T &m_mutex;
        bool m_locked = false;
    };

    // 写锁模板
    template <typename T>
    class WriteScopedlockedImpl
    {
    public:
        WriteScopedlockedImpl(T &mutex) : m_mutex(mutex)
        {
            lock();
        }
        ~WriteScopedlockedImpl()
        {
            unlock();
        }

        /**
         * @brief 尝试获取锁
         * @return if successful, return 0
         */
        int try_lock()
        {
            int ret = m_mutex.try_lock();
            if (ret == 0)
            {
                m_locked = true;
            }
            return ret;
        }

        /**
         * @brief 获取锁
         * @return
         */
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.lock();
                m_locked = true;
            }
        }

        /**
         * @brief 解锁
         * @return
         */
        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    public:
        WriteScopedlockedImpl(const WriteScopedlockedImpl &) = delete;
        WriteScopedlockedImpl &operator=(const WriteScopedlockedImpl &) = delete;

    private:
        T &m_mutex;
        bool m_locked = false;
    };

    class Mmutex : public std::mutex
    {
    public:
        using lockSco = ScopedlockedImpl<std::mutex>;
    };

    class SpinLockMutex : public std::mutex
    {
    public:
        using lockSco = ScopedlockedImpl<std::mutex>;
        using WritelockSco = ScopedlockedImpl<std::mutex>;
    };

    class MRWmutex : public std::shared_mutex
    {
    public:
        using ReadlockSco = ReadScopedlockedImpl<std::shared_mutex>;
        using WritelockSco = WriteScopedlockedImpl<std::shared_mutex>;
    };

    // 线程
    class Mthread
    {
    public:
        using MthreadPtr = std::shared_ptr<Mthread>;

        Mthread(std::function<void()> cb, const std::string &name);
        ~Mthread();

        /**
         * @brief 获取线程id
         * @return pid
         */
        pid_t getID() const { return m_id; }

        /**
         * @brief 获取线程名称
         * @return 线程名称
         */
        const std::string &getName() const { return m_name; }

        /**
         * @brief 线程会和
         * @return
         */
        void join();

        /**
         * @brief 分离
         */
        void detach();

        /**
         * @brief 获取当前线程
         * @return 当前线程的裸指针
         */
        static Mthread *GetThis();

        /**
         * @brief 获取线程名称
         * @return 线程名称
         */
        static const std::string &GetName();

        /**
         * @brief 设置线程名称
         * @param name 新的线程名称
         * @return
         */
        static void SetThreadName(const std::string &name);

    private:
        /**
         * @brief 线程执行函数
         * @param arg 需要执行的线程指针
         * @return nullptr
         */
        static void run(Mthread *);

    public:
        Mthread(const Mthread &) = delete;
        Mthread(Mthread &&) = delete;
        Mthread &operator=(const Mthread &) = delete;

    private:
        std::mutex m_mutex;                 // mutex
        pid_t m_id = -1;                    // 线程id
        std::thread m_thread;               // 线程identify
        std::function<void()> m_cb;         // 回调函数
        std::string m_name;                 // 线程名称
        std::condition_variable m_cv;       // 条件变量
        std::atomic<bool> m_running{false}; // 原子检测搭配cv使用
    };
}
#endif // __BLUE_MTHREAD_H__