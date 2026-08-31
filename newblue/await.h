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
#pragma once
#include <coroutine>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include "blue/log.h"
#include "newblue/io_manager.h"

namespace newblue
{
    blue::Logger::LoggerPtr s_logger = BLUE_LOG_NAME("system");
    struct SleepAwaiter
    {
        uint64_t ms;

        SleepAwaiter(uint64_t ms):ms(ms) {}

        bool await_ready() const noexcept 
        { 
            return false; 
        }

        void await_suspend(std::coroutine_handle<> h) 
        {
            IOManager::GetThis()->addTimer(ms,h,nullptr);
        }

        void await_resume() const noexcept {}
    };
    SleepAwaiter sleepFor(uint64_t ms)
    {
        return SleepAwaiter{ms};
    }
}