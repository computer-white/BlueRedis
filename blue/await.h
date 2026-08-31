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