#pragma once  

#include <iostream>       // 标准输入输出（辅助调试）
#include <string>         // 字符串处理（存储设备路径）
#include <sys/types.h>    // 系统类型定义（如pid_t）
#include <unistd.h>       // POSIX系统调用（如fork、setsid、chdir）
#include <signal.h>       // 信号处理（如signal、SIG_IGN）
#include <sys/stat.h>     // 文件状态操作（辅助open函数）
#include <fcntl.h>        // 文件控制（如open函数的标志位）

#include "Log.hpp"        // 日志模块 ---> 记录错误信息
#include "Common.hpp"     // 公共常量 ---> 如OPEN_ERR错误码

//1.引入日志模块命名空间，简化日志调用
using namespace LogModule;  
//2.空设备路径：所有写入的数据都会被丢弃
const std::string dev = "/dev/null";  

// 将当前进程转换为守护进程（Daemon）
void Daemon(int nochdir, int noclose)
{
    //1.忽略无关信号，避免守护进程被意外终止
    signal(SIGPIPE, SIG_IGN);   // SIGPIPE：向已关闭的管道写入数据时产生，忽略避免进程退出
    signal(SIGCHLD, SIG_IGN);   // SIGCHLD：子进程退出时产生，忽略避免产生僵尸进程

    //2.创建子进程并让父进程退出，确保进程不是会话组长
    if (fork() > 0)  // 父进程分支
    {
        exit(0);     // 父进程退出，子进程成为孤儿进程（由init进程收养）
    }

    //3.子进程创建新的会话，脱离原终端控制
    setsid();  // 此时进程不再关联任何终端，成为后台进程

    //4.切换工作目录到根目录（避免依赖原工作目录）
    //4.1：若nochdir为0，则切换到根目录"/"（根目录通常不会被卸载）
    if(nochdir == 0)  
    {
        chdir("/");  
    }
    //4.2：若nochdir非0，则保留当前工作目录

    //5.重定向标准输入/输出/错误到空设备 ---> 守护进程通常不需要与终端交互，因此关闭或重定向标准流(注意：不推荐直接关闭0,1,2文件描述符)
    if (noclose == 0)  // 若noclose为0，则执行重定向
    {
        //5.1：打开空设备/dev/null（可读可写）
        int fd = ::open(dev.c_str(), O_RDWR); //注意：/dev/null特性：读取返回空，写入的数据被丢弃（类似"黑洞"）

        //情况一：打开失败
        if (fd < 0)  
        {
            LOG(LogLevel::FATAL) << "open " << dev << " errno";  
            exit(OPEN_ERR);  
        }

        //情况二：打开成功，重定向标准流
        else  
        {
            //1.将标准输入/输出/错误重定向到/dev/null
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);

            //2.关闭原始的/dev/null文件描述符（已通过dup2复制）
            close(fd);  
        }
    }
}