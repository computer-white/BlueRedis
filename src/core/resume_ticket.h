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
 * @file resume_ticket.h
 * @brief 协程句柄还能不能 resume 的抽象封装
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.11.1
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <coroutine>
#include <atomic>
#include <memory>

namespace blue
{
    // 由于 std::coroutine_handle 没有感知能力，无法知道协程是否以及done/destroy了
    // 所以需要对 handle 进行抽象，来保证多线程下，定时器 / IOManager 共同持有一个
    // handle 时不能多次 resume
    struct ResumeTicket
    {
        std::coroutine_handle<> m_handle = nullptr; // 协程句柄
        std::atomic<bool> m_armed{false};           // 协程还是否可以 resume
    };

    struct AsyncIoState
    {
        std::atomic<bool> is_timeout{false};
        std::atomic<bool> event_added{false};
    };

    void try_resume(const std::shared_ptr<ResumeTicket> &rt);
}