/**
 * @file await.h
 * @brief 基于协程和调度器的对于sleep的改写
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.2
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <coroutine>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include "log.h"
#include "io_manager.h"

namespace blue
{
    struct SleepAwaiter
    {
        uint32_t s;

        SleepAwaiter(uint32_t s) : s(s) {}

        bool await_ready() const noexcept
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> h)
        {
            IOManager::GetThis()->addTimer(s * 1000, h, nullptr);
        }

        void await_resume() const noexcept {}
    };

    /**
     * @brief 等待 s 秒
     */
    inline SleepAwaiter sleepFor(uint32_t s)
    {
        return SleepAwaiter{s};
    }


    struct SleepAwaiterMs 
    {
        uint64_t ms;
        
        SleepAwaiterMs(uint64_t ms) : ms(ms) {}
        
        bool await_ready() const noexcept { return false; }
        
        void await_suspend(std::coroutine_handle<> h) {
            IOManager::GetThis()->addTimer(ms, h, nullptr);
        }
        
        void await_resume() const noexcept {}
    };

    inline SleepAwaiterMs sleepForMs(uint64_t ms) {
        return SleepAwaiterMs{ms};
    }
}