#include <dlfcn.h>
#include <stdarg.h>
#include "config.h"
#include "fdmanager.h"
#include "hook.h"
#include "io_manager.h"
#include "log.h"

// hook
namespace blue
{
	static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

	static blue::ConfigVar<int>::ConfigVarPtr g_tcp_connect_timeout =
		blue::Config::Lookup<int>("tcp.connect.timeout", 10000u, "tcp connect timeout");

	static thread_local bool t_hook_enable = false;
#define HOOK_FUNC(XX) \
	XX(socket)        \
	XX(close)         \
	XX(fcntl)         \
	XX(ioctl)         \
	XX(getsockopt)    \
	XX(setsockopt)

	// hook初始化
	void hook_initial()
	{
		static bool is_inited = false;
		if (is_inited)
		{
			return;
		}
#define XX(name) name##_f = (name##_func)dlsym(RTLD_NEXT, #name);
		HOOK_FUNC(XX);
#undef XX
	}
	static uint64_t s_connect_timeout = -1;
	// ----main函数前会进行初始化
	struct __HookIniter__
	{
		__HookIniter__()
		{
			hook_initial();
			s_connect_timeout = g_tcp_connect_timeout->getValue();

			g_tcp_connect_timeout->addListener([](const int &old_val, const int &new_val)
											   {
				BLUE_LOG_INFO(g_logger) << "tc connect timeout changed " 
										<<"old_val : " << old_val << " new_val " << new_val;
				s_connect_timeout = new_val; });
		}
	};

	static __HookIniter__ __S_Hook_Initer__;
	// ----main函数前会进行初始化

	// 是否hook
	bool is_hook_enable()
	{
		return t_hook_enable;
	}

	// 设置hook
	void set_hook_enable(bool flag)
	{
		t_hook_enable = flag;
	}

	struct TimerInfo
	{
		std::atomic<int> cancelled = {0};
	};

	extern "C"
	{
#define XX(name) name##_func name##_f = nullptr;
		HOOK_FUNC(XX);
#undef XX
		
		// hook socket相关
		int socket(int domain, int type, int protocol)
		{
			if (!blue::is_hook_enable())
			{
				return socket_f(domain, type, protocol);
			}
			// BLUE_LOG_INFO(g_logger) << "socket hook successfuly";
			int fd = socket_f(domain, type, protocol);
			if (fd == -1)
			{
				BLUE_LOG_ERROR(g_logger) << "socket failed";
				return fd;
			}
			// 这里会添加(没有就创建)
			// BLUE_LOG_INFO(g_logger) << "socket successful";
			blue::FdManagerPtr::GetInstance()->get(fd, true);
			return fd;
		}
		
		// hook close相关
		int close(int fd)
		{
			if (!blue::is_hook_enable())
			{
				return close_f(fd);
			}
			// BLUE_LOG_INFO(g_logger) << "close hook successfuly";
			blue::FdCxt::FdCxtPtr ctx = blue::FdManagerPtr::GetInstance()->get(fd);
			if (ctx)
			{
				auto iom = blue::IOManager::GetThis();
				if (iom)
				{
					iom->cancelAll(fd);
				}
				// 删除fd相关的事件
				blue::FdManagerPtr::GetInstance()->del(fd);
			}
			return close_f(fd);
		}

		// hook set/get socket option 相关
		int fcntl(int fd, int cmd, ... /* arg */)
		{
			va_list va;
			va_start(va, cmd);
			switch (cmd)
			{
			case F_SETFL:
			{
				int arg = va_arg(va, int);
				va_end(va);
				blue::FdCxt::FdCxtPtr cxt = blue::FdManagerPtr::GetInstance()->get(fd);
				if (!cxt || cxt->isClosed() || !cxt->isSocket())
				{
					return fcntl_f(fd, cmd, arg);
				}
				// 设置用户nonblock
				cxt->setUserNonBlock(arg & O_NONBLOCK);
				if (cxt->getSysNonBlock())
				{
					arg |= O_NONBLOCK;
				}
				else
				{
					arg &= ~O_NONBLOCK;
				}
				return fcntl_f(fd, cmd, arg);
			}
			break;
			case F_GETFL:
			{
				va_end(va);
				int ans = fcntl_f(fd, cmd);
				blue::FdCxt::FdCxtPtr cxt = blue::FdManagerPtr::GetInstance()->get(fd);
				if (!cxt || cxt->isClosed() || !cxt->isSocket())
				{
					return ans;
				}
				if (cxt->getUserNonBlock())
				{
					return ans | O_NONBLOCK;
				}
				else
				{
					return ans & ~O_NONBLOCK;
				}
			}
			break;
			case F_DUPFD:
			case F_DUPFD_CLOEXEC:
			case F_SETFD:
			case F_SETOWN:
			case F_SETSIG:
			case F_SETLEASE:
			case F_NOTIFY:
			case F_SETPIPE_SZ:
			{
				int arg = va_arg(va, int);
				va_end(va);
				return fcntl_f(fd, cmd, arg);
			}
			break;

			case F_GETFD:
			case F_GETOWN:
			case F_GETSIG:
			case F_GETLEASE:
			case F_GETPIPE_SZ:
			{
				va_end(va);
				return fcntl_f(fd, cmd);
			}
			break;

			case F_SETLK:
			case F_SETLKW:
			case F_GETLK:
			{
				struct flock *arg = va_arg(va, struct flock *);
				va_end(va);
				return fcntl_f(fd, cmd, arg);
			}
			break;
			case F_GETOWN_EX:
			case F_SETOWN_EX:
			{
				struct f_owner_ex *arg = va_arg(va, struct f_owner_ex *);
				va_end(va);
				return fcntl_f(fd, cmd, arg);
			}
			break;
			default:
				va_end(va);
				return fcntl_f(fd, cmd);
			}
		}

		int ioctl(int fd, unsigned long request, ...)
		{
			va_list va;
			va_start(va, request);
			void *arg = va_arg(va, void *);
			va_end(va);
			if (request == FIONBIO)
			{
				bool user_nonblock = (*((int *)(arg)) != 0);
				blue::FdCxt::FdCxtPtr cxt = blue::FdManagerPtr::GetInstance()->get(fd);
				if (!cxt || cxt->isClosed() || !cxt->isSocket())
				{
					return ioctl_f(fd, request, arg);
				}
				cxt->setUserNonBlock(user_nonblock);
			}
			return ioctl_f(fd, request, arg);
		}

		int getsockopt(int sockfd, int level, int optname,
					   void *optval, socklen_t *optlen)
		{
			return getsockopt_f(sockfd, level, optname, optval, optlen);
		}

		int setsockopt(int sockfd, int level, int optname,
					   const void *optval, socklen_t optlen)
		{
			if (!blue::is_hook_enable())
			{
				return setsockopt_f(sockfd, level, optname, optval, optlen);
			}
			// BLUE_LOG_INFO(g_logger) << "setsockopt hook successfuly";
			if (level == SOL_SOCKET)
			{
				if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
				{
					blue::FdCxt::FdCxtPtr cxt = blue::FdManagerPtr::GetInstance()->get(sockfd);
					if (cxt)
					{
						struct timeval *tv = (struct timeval *)optval;
						cxt->setTimeout(optname, tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000);
					}
				}
			}
			return setsockopt_f(sockfd, level, optname, optval, optlen);
		}
	}
#undef HOOK_FUNC
}
