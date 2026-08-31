/*
 * BlueRedis - High Performance Redis Server based on C++20 Coroutine
 * Copyright (C) 2026 blue
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <exception>
#include "mthread.h"
#include "log.h"
#include "util.h"

#ifdef __linux__
#include <pthread.h>
#endif

// 线程模块
namespace blue
{
    static thread_local Mthread *t_thread = nullptr;
    static thread_local std::string t_thread_name = "NOKNOW";
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    Mthread *Mthread::GetThis()
    {
        return t_thread;
    }

    const std::string &Mthread::GetName()
    {
        return t_thread_name;
    }

    void Mthread::run(Mthread *thread)
    {
        t_thread = thread;
        t_thread_name = thread->m_name;
        thread->m_id = blue::GetThreadId();
        // pthread_self()返回tid
#ifdef __linux__
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
#endif

        std::function<void()> cb;
        cb.swap(thread->m_cb);

        // 唤醒
        thread->m_running.store(true, std::memory_order_release);
        thread->m_cv.notify_one();

        cb();
        return;
    }

    void Mthread::SetThreadName(const std::string &name)
    {
        if (t_thread)
        {
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }

    Mthread::Mthread(std::function<void()> cb, const std::string &name) : m_cb(cb),
                                                                          m_name(name)
    {
        if (name.empty())
        {
            m_name = "NOKNOW";
        }
        m_thread = std::thread(&Mthread::run,this);
        // 等待run函数把一切都设置好
        std::unique_lock<std::mutex> lk(m_mutex);
        m_cv.wait(lk,[this]{return m_running.load(std::memory_order_acquire); });
    }
    Mthread::~Mthread()
    {
        if (m_thread.joinable())
        {
            m_thread.detach();
        }
    }

    void Mthread::join()
    {
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void Mthread::detach()
    {
        if (m_thread.joinable())
        {
            m_thread.detach();
        }
    }

}