#include "scheduler.h"
#include "blue/log.h"
#include "blue/macro.h"

// 协程调度模块:分配协程到对应的线程
namespace newblue
{
    static thread_local Scheduler* t_Scheduler = nullptr;

    Scheduler::Scheduler(size_t threads, const std::string& name)
    {
        BLUE_ASSERT(threads > 0);
        m_name = name;
        m_threadCount = threads;
    }

    Scheduler::~Scheduler()
    {
        BLUE_ASSERT(m_stop.load(std::memory_order_acquire));
    }

    void Scheduler::start()
    {
        if (!m_stop.load(std::memory_order_acquire))
        {
            return;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_stop.load(std::memory_order_acquire))
        {
            return;
        }
        m_stop.store(false,std::memory_order_release);
        for (size_t i = 0; i < m_threadCount; i++) 
        {
            m_workers.emplace_back(std::bind(&Scheduler::run,this));
        }
    }

    void Scheduler::stop()
    {
        m_stop.store(true,std::memory_order_release);
        m_cv.notify_all();
        for (auto& worker : m_workers) 
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }

    void Scheduler::schedule(Task task , int thr)
    {
        auto t = std::make_shared<Task>(std::move(task));
        if (*t && t->done()) return;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.push_back(t);
        }
        schedule([t](){
            if (t && !t->done())
            {
                t->resume();
            }
        },thr);
    }

    void Scheduler::schedule(std::coroutine_handle<> h)
    {
        if (h && h.done()) 
        {
            return;
        }
        schedule([h]() {
            if (h && !h.done())
            {
                h.resume();
            }
        });
    }

    void Scheduler::schedule(std::function<void()> cb, int thr)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            FuncAndId tem(cb,thr);
            m_queue.push(tem);
            m_pending.fetch_add(1,std::memory_order_acq_rel);
        }
        m_cv.notify_one();
        tickle();
    }

    void Scheduler::run()
    {
        setThis(this);
        BLUE_ASSERT(t_Scheduler);
        while (true)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock,[this]{return m_stop.load(std::memory_order_acquire) || !m_queue.empty(); } );
            if (m_stop.load(std::memory_order_acquire) && m_queue.empty())
            {
                break;
            }
            auto it = m_queue.front();
            m_queue.pop();
            lock.unlock();
            if (it.cb) 
            {
                it.cb();
            }
            m_pending.fetch_sub(1,std::memory_order_acq_rel);
            m_cv.notify_one();
        }
    }

    void Scheduler::tickle()
    {
        
    }

    bool Scheduler::stopping()
    {
        bool res = m_stop.load(std::memory_order_acquire);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            res = res && m_queue.empty();
        }
        return res;
    }

    void Scheduler::wait_all()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock,[this]{return m_pending == 0; });
    }

    Scheduler *Scheduler::GetThis()
    {
        BLUE_ASSERT(t_Scheduler);
        return t_Scheduler;
    }

    void Scheduler::setThis(Scheduler* t)
    {
        t_Scheduler = t;
    }


}