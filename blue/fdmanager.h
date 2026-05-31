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
         * @return
         */
        ~FdCxt();

        /**
         * @brief 初始化私有变量函数
         * @return
         */
        bool init();

        /**
         * @brief 是否初始化
         * @return 初始化了返回true 否则返回fasle
         */
        bool isInit() const { return m_isInit; }

        /**
         * @brief 是否说socket文件描述符
         * @return 是返回true 否则返回fasle
         */
        bool isSocket() const { return m_isSocket; }

        /**
         * @brief 文件描述符是否被关闭
         * @return 关闭了返回true 否则返回fasle
         */
        bool isClosed() const { return m_isClosed; }

        /**
         * @brief 获取用户是否自行设置非阻塞
         * @return 是返回true 否则返回fasle
         */
        bool getUserNonBlock() const { return m_UserNonBlock; }

        /**
         * @brief 设置用户是否自行设置非阻塞
         * @return
         */
        void setUserNonBlock(bool val) { m_UserNonBlock = val; }

        /**
         * @brief 获取系统是否设置非阻塞
         * @return 是返回true 否则返回fasle
         */
        bool getSysNonBlock() const { return m_SysNonBlock; }

        /**
         * @brief 设置系统是否自行设置非阻塞
         * @return
         */
        void setSysNoBlock(bool val) { m_SysNonBlock = val; }

        /**
         * @brief 设置超时
         * @param type 超时事件类型(读/写)
         * @param val 超时时间(ms)
         * @return
         */
        void setTimeout(int type, uint64_t val);

        /**
         * @brief 设置 isclosed
         * @param val true 表示关闭
         */
        void setClosed(bool val) { m_isClosed = val; }

        /**
         * @brief 获取超时
         * @param type 超时事件类型(读/写)
         * @param val 超时时间(ms)
         * @return 超时时间(ms)
         */
        uint64_t getTimeout(int type);

    private:
        bool m_isInit : 1;
        bool m_isSocket : 1;
        bool m_SysNonBlock : 1;
        bool m_UserNonBlock : 1;
        bool m_isClosed : 1;
        uint64_t m_revTimeout;
        uint64_t m_sendTimeout;
        int m_fd;
    };
    class FdManager
    {
    public:
        using MRWmutexType = MRWmutex;

    public:
        // FdManager(int fd);

        /**
         * @brief FdManger构造函数
         * @return
         */
        FdManager();

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
        // std::vector<FdCxt::FdCxtPtr> m_datas;   // 按照fd作为数组下标,使用时保证fd不要跨度很大
        std::unordered_map<int, FdCxt::FdCxtPtr> m_datas; // 选择使用map存储,fd描述符可能出现不连续
    };

    // 单例模式
    using FdManagerPtr = SingleTonPtr<FdManager>;
}
#endif