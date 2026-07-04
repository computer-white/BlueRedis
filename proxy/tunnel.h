/**
 * @file tunnel.h
 * @brief 管道模块,用于httpserver中隧道连接和websocket
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.19
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include "blue/msocket.h"
#include "blue/task.h"

namespace blue
{
    namespace proxy
    {
        class Tunnel
        {
        public:
            static Task<void> create(MSocket::MSocketPtr from, MSocket::MSocketPtr to)
            {
                char buf[16384];
                int fd_from = from->getSocketfd();
                int fd_to = to->getSocketfd();

                while (true)
                {
                    // from → to
                    ssize_t n = co_await Recv(fd_from, buf, sizeof(buf), 0);
                    if (n <= 0)
                    {
                        break;
                    }

                    ssize_t sent = 0;
                    while (sent < n)
                    {
                        ssize_t ret = co_await Send(fd_to, buf + sent, n - sent, 0);
                        if (ret <= 0)
                        {
                            break;
                        }
                        sent += ret;
                    }
                    if (sent < n)
                    {
                        break;
                    }

                    // to → from
                    n = co_await Recv(fd_to, buf, sizeof(buf), 0);
                    if (n <= 0)
                    {
                        break;
                    }

                    sent = 0;
                    while (sent < n)
                    {
                        ssize_t ret = co_await Send(fd_from, buf + sent, n - sent, 0);
                        if (ret <= 0)
                        {
                            break;
                        }
                        sent += ret;
                    }
                    if (sent < n)
                    {
                        break;
                    }
                }

                co_return;
            }
        };
    }
}