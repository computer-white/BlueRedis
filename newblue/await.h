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