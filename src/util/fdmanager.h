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
 * @file fdmanager.h
 * @brief 文件描述符fd管理
 * @authors blue
 * @email homeheyang@outlook.com
 * @date 2026.5.1
 * @copyright Copyright (c) 2026年 blue
 */
#ifndef BLUE_FDMANAGER_H
#define BLUE_FDMANAGER_h
#include <memory>
#include <unordered_map>
#include "mthread.h"
#include "singleton.h"

// 文件描述符fd管理
namespace blue
{
    class FdCxt : std::enable_shared_from_this<FdCxt>
    {
    public:
        using FdCxtPtr = std::shared_ptr<FdCxt>;

    public:
        /**
         * @brief 文件描述符内容构造函数
         * @param fd 文件描述符句柄
         * @return
         */
        FdCxt(int fd);

        /**
         * @brief 文件描述符内容析构函数
         */
        ~FdCxt() = default;

        /**
         * @brief 初始化私有变量函数
         */
        bool init();

        /**
         * @brief 是否说socket文件描述符
         */
        bool isSocket() const { return m_isSocket; }

        /**
         * @brief 文件描述符是否被关闭
         */
        bool isClosed() const { return m_isClosed; }

        /**
         * @brief 设置 isclosed
         * @param val true 表示关闭
         */
        void setClosed(bool val) { m_isClosed = val; }

    private:
        bool m_isInit : 1;
        bool m_isSocket : 1;
        bool m_isClosed : 1;
        int m_fd;
    };
    class FdManager
    {
    public:
        using MRWmutexType = MRWmutex;

    public:
        /**
         * @brief FdManger构造函数
         * @return
         */
        FdManager() = default;

        /**
         * @brief 获取文件描述符对应的FdCxt指针
         * @param fd 文件描述符
         * @param auto_create true表示没有旧创建新的 默认 fasle
         * @return FdCxt智能指针
         */
        FdCxt::FdCxtPtr get(int fd, bool auto_create = false);

        /**
         * @brief 根据文件描述符删除FdCxt
         * @param fd 文件描述符
         * @return
         */
        void del(int fd);

    private:
        MRWmutexType m_mutex;
        std::unordered_map<int, FdCxt::FdCxtPtr> m_datas; // 选择使用map存储,fd描述符可能出现不连续
    };

    // 单例模式
    using FdManagerPtr = SingleTonPtr<FdManager>;
}
#endif