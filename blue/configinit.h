/**
 * @file configinit.h
 * @brief 一些数据库和redis配置信息
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.10
 * @copyright Copyright (c) 2026年 blue
 */
#ifndef BLUE_CONFIGINIT_H
#define BLUE_CONFIGINIT_H
#include "dbmanager.h"
#include "redismanager.h"
#include <string>

// 数据库和redis配置
namespace blue
{
    void blueIniteConfig();     // 初始化blue命名空间中的配置信息
    namespace http
    {
        void blueHttpIniteConfig();                    // 初始化函数,使用代理时必须先初始化mysql和redis
    }
}

#endif