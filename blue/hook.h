/**
 * @file hook.h
 * @brief 对于一些api的hook封装,之前跟随b站sylar写的，目前已经用不上了
 * @authors blue
 * @email homeheyang@outlook.com
 * @date 2026.5.1
 * @copyright Copyright (c) 2026年 blue
 */
#ifndef BLUE_HOOK_H
#define BLUE_HOOK_H
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
// 同步原语异步化

// 将系统调用的sleep()拦截,变为我们自己实现的sleep()执行一个特定时间后提交任务的函数
// 而后调用YieldToHold(),从而让线程让出cpu

// hook
namespace blue
{
	bool is_hook_enable();
	void set_hook_enable(bool flag);
	// int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t len, uint64_t timeout);
}
extern "C"
{
	// socket
	typedef int (*socket_func)(int domain, int type, int protocol);
	extern socket_func socket_f;

	// close
	typedef int (*close_func)(int fd);
	extern close_func close_f;

	// socket operation
	typedef int (*fcntl_func)(int fd, int cmd, ... /* arg */);
	extern fcntl_func fcntl_f;

	typedef int (*ioctl_func)(int fd, unsigned long request, ...);
	extern ioctl_func ioctl_f;

	typedef int (*getsockopt_func)(int sockfd, int level, int optname,
								   void *optval, socklen_t *optlen);
	extern getsockopt_func getsockopt_f;

	typedef int (*setsockopt_func)(int sockfd, int level, int optname,
								   const void *optval, socklen_t optlen);
	extern setsockopt_func setsockopt_f;
}
#endif // __BLUE_HOOK_H
