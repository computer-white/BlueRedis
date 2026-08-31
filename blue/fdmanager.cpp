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
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include "fdmanager.h"
#include "log.h"
#include "hook.h"

// 文件描述符fd管理
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    FdCxt::FdCxt(int fd)
        : m_isInit(false),
          m_isSocket(false),
          m_SysNonBlock(false),
          m_UserNonBlock(false),
          m_isClosed(false),
          m_revTimeout(UINT64_MAX),
          m_sendTimeout(UINT64_MAX),
          m_fd(fd)
    {
        init();
    }

    FdCxt::~FdCxt()
    {
        // BLUE_LOG_INFO(g_logger) << "FdCxt destructor, fd=" << m_fd << ", this=" << this;
    }

    bool FdCxt::init()
    {
        if (m_isInit)
        {
            return true;
        }
        struct stat buf;
        // 检查文件状态(是否是socket描述符)
        if (fstat(m_fd, &buf) == -1)
        {
            m_isInit = false;
            m_isSocket = false;
        }
        else
        {
            m_isInit = true;
            m_isSocket = S_ISSOCK(buf.st_mode);
        }

        if (m_isSocket)
        {
            // 直接调用系统的不要去被hook了
            int flags = fcntl_f(m_fd, F_GETFL, 0);
            // 不包含非阻塞模式
            if (!(flags & O_NONBLOCK))
            {
                // 设置为非阻塞模式
                fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
            }
            m_SysNonBlock = true;
        }
        else
        {
            m_SysNonBlock = false;
        }
        m_UserNonBlock = false;
        m_isClosed = false;
        return m_isInit;
    }

    void FdCxt::setTimeout(int type, uint64_t val)
    {
        if (type == SO_RCVTIMEO)
        {
            m_revTimeout = val;
        }
        else
        {
            m_sendTimeout = val;
        }
    }

    uint64_t FdCxt::getTimeout(int type)
    {
        if (type == SO_RCVTIMEO)
        {
            return m_revTimeout;
        }
        return m_sendTimeout;
    }

    FdManager::FdManager()
    {
    }

    FdCxt::FdCxtPtr FdManager::get(int fd, bool auto_create)
    {
        {
            MRWmutexType::ReadlockSco rlock(m_mutex);
            auto it = m_datas.find(fd);
            if (it != m_datas.end())
                return it->second;
            // 不存在也不需要创建新的
            if (!auto_create)
                return nullptr;
        }
        // 不存在,并且需要创建新的
        MRWmutexType::WritelockSco wlock(m_mutex);
        auto it = m_datas.find(fd);
        if (it != m_datas.end())
        {
            return it->second;
        }

        // FdCxt::FdCxtPtr newFdcxt = std::make_shared<FdCxt>(fd);
        FdCxt::FdCxtPtr newFdcxt(new FdCxt(fd));
    
        m_datas[fd] = newFdcxt;
        return m_datas[fd];
    }

    void FdManager::del(int fd)
    {
        MRWmutexType::WritelockSco lock(m_mutex);
        auto it = m_datas.find(fd);
        if (it == m_datas.end())
        {
            BLUE_LOG_INFO(g_logger) << "需要删除的文件fd不存在";
            return;
        }
        it->second->setClosed(true);
        m_datas.erase(it);
    }
}