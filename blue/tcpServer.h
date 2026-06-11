#ifndef BLUE_TCPSERVER_H
#define BLUE_TCPSERVER_H
#include <memory>
#include <functional>
#include "address.h"
#include "io_manager.h"
#include "msocket.h"
#include "task.h"

// tcp server
namespace blue
{
    /* 对TCP 服务的封装 */
    template <typename T>
    class TcpServer : public std::enable_shared_from_this<TcpServer<T>>
    {
    public:
        using TcpServerPtr = std::shared_ptr<TcpServer<T>>;
        TcpServer(const TcpServer &) = delete;
        TcpServer &operator=(const TcpServer &) = delete;

    public:
        /**
         * @brief 构造一个TcpServer对象
         * @param level 协议例如SOL_SOCKET 默认 -1
         * @param option_name 选项名称 例如SO_REUSEADDR 默认 -1
         * @param option 选项值 1开启 0禁用 默认 0
         * @param manager 工作iomanager
         * @param acceptmanager 处理accpet的iomanager
         * @note 默认已经设置了SO_REUSEADDR, SO_RESUEPORT, TCP_NODELAY,默认超时时长2分钟
         */
        TcpServer(int level = -1, int option_name = -1, T option = T(), IOManager *manager = IOManager::GetThis(),
                  IOManager *acceptmanager = IOManager::GetThis());

        /**
         * @brief 虚析构函数,关闭所有绑定的socket
         */
        virtual ~TcpServer();

        /**
         * @brief bind一个地址
         * @param address 需要bind的地址
         * @return 成功返回true
         * @note bind成功紧接着直接进行listen
         */
        virtual bool bind(Address::AddressPtr address);

        /**
         * @brief bind 一组地址
         * @param address 需要绑定的一组地址
         * @param fails 返回失败的地址
         * @return 成功返回true
         * @note 若有地址错误,将不会有任何地址被绑定.bind成功紧接着直接listen
         */
        virtual bool bind(std::vector<Address::AddressPtr> &address, std::vector<Address::AddressPtr> &fails);

        /**
         * @brief start 开始tcpserver
         * @return 成功返回true
         */
        virtual Task<bool> start();

        /**
         * @brief stop 停止tcpserver
         * @return 成功返回true
         * @note 取消socket上绑定的事件并关闭socket
         */
        virtual Task<bool> stop();

        /**
         * @brief 获取receive超时
         * @return 返回receive超时时间
         */
        uint64_t getRecvTimeOut() const noexcept { return m_RecvTimeOut; }

        /**
         * @brief 获取tcp服务名称
         * @return 返回tcp服务名称
         */
        std::string getName() const noexcept { return m_name; }

        /**
         * @brief 设置receive超时时间
         * @param timeout 需要设置的超时时间(ms)
         */
        void setRecvTimeOut(uint64_t timeout) noexcept { m_RecvTimeOut = timeout; }

        /**
         * @brief 设置tcp服务名称
         * @param name tcp服务名称
         */
        void setName(const std::string &name) noexcept { m_name = name; }

        /**
         * @brief tcp是否停止
         * @return true表示停止
         */
        bool getIsStop() const noexcept { return m_isStop.load(std::memory_order_acquire); }

        /**
         * @brief 增加连接数
         */
        void addConnection() noexcept { m_connections.fetch_add(1); }

        /**
         * @brief 减少连接
         */
        void subConnection() noexcept { m_connections.fetch_sub(1); }

        /**
         * @brief 获取当前连接数量
         * @return 当前连接数量
         */
        uint32_t getConnection() const noexcept { return m_connections.load(std::memory_order_acquire); }
    protected:
        /**
         * @brief 处理client事件
         * @param sock 客户端 socket fd
         */
        virtual Task<void> handleClient(MSocket::MSocketPtr sock);

        /**
         * @brief 开始accept
         * @param sock listen socket fd
         */
        virtual Task<void> startAccept(MSocket::MSocketPtr sock);

    private:
        std::vector<blue::MSocket::MSocketPtr> m_socks;
        IOManager *m_worker;
        IOManager *m_acceptworker;
        std::string m_name;
        uint64_t m_RecvTimeOut;
        int m_level;
        int m_option_name;
        T m_option;
        std::atomic<bool> m_isStop = {true};
        std::atomic<uint32_t> m_connections{0};
    };
}

#endif
