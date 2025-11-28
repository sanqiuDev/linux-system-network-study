#pragma once

#include <cstdio>  //包含“标准输入输出库头文件” ---> 提供perror函数（用于打印系统错误信息）的声明
#include <cstdlib> //包含“标准库头文件” ----------> 提供exit函数（用于终止程序）和EXIT_FAILURE宏（表示程序退出的失败状态）的声明

// 定义错误处理宏ERR_EXIT，用于简化系统调用或函数出错时的处理流程
#define ERR_EXIT(m)         \
    do                      \
    {                       \
        perror(m);          \
        exit(EXIT_FAILURE); \
    } while (0)
