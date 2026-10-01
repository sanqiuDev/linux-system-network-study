#pragma once  
#include <iostream>               
#include <unistd.h>              
#include <sys/epoll.h>   // epoll核心头文件（定义epoll_create/ctl/wait等接口）

#include "Common.hpp"            
#include "Log.hpp"              
using namespace LogModule;      

// Epoller类：对Linux epoll内核事件模型的封装
class Epoller // 设计目标：解耦Reactor与epoll内核交互，便于后续扩展为Poll/Select等多路复用模型
{
private:
    //1.epoll内核实例句柄（核心：标识一个epoll模型）
    int _epfd;  

public:
    //1.“构造函数” ---> 创建epoll内核实例（epoll_fd）
    Epoller() : _epfd(-1)  // 初始化epoll句柄为无效值（-1）
    {
        //1.创建epoll实例
        _epfd = epoll_create(128);
        if (_epfd < 0)  
        {
            LOG(LogLevel::FATAL) << "epoll_create error!!!";  
            exit(EPOLL_CREATE_ERR);  
        }
        LOG(LogLevel::INFO) << "create epoll success: " << _epfd;
    }

    //2.“析构函数” ---> 释放epoll内核实例资源
    ~Epoller()
    {
        //1.确保epoll句柄有效（避免重复关闭）
        if (_epfd >= 0) 
        {
            close(_epfd);  // 关闭epoll句柄，释放内核资源
        }
    }

    /*----------------------------------------------【内部辅助函数】----------------------------------------------*/
    //1.epoll_ctl通用操作封装（添加/删除/修改事件的核心逻辑）
    void ModEventHelper(int sockfd, uint32_t events, int oper)
    {
        //1.描述fd和事件的结构体
        struct epoll_event ev;  

        //2.设置关注的事件类型
        ev.events = events;     

        //3.绑定fd到事件结构体（关键：epoll通过此关联fd和事件）
        ev.data.fd = sockfd;    

        //4.修改内核中的事件监听列表 ---> 调用epoll_ctl操作epoll实例
        int n = epoll_ctl(_epfd, oper, sockfd, &ev);
        if (n < 0)
        {
            LOG(LogLevel::ERROR) << "epoll_ctl error";  
            return;
        }
        LOG(LogLevel::INFO) << "epoll_ctl success: " << sockfd;  
    }


    /*----------------------------------------------【对外功能接口】----------------------------------------------*/
    //1.添加fd及关注的事件到epoll实例 ---> 调用辅助函数，执行EPOLL_CTL_ADD操作
    void AddEvent(int sockfd, uint32_t events)
    {
        ModEventHelper(sockfd, events, EPOLL_CTL_ADD);
    }

    //2.从epoll实例中删除fd的事件监听
    void DelEvent(int sockfd)
    {
        int n = epoll_ctl(_epfd, EPOLL_CTL_DEL, sockfd, nullptr); // 注意：删除操作无需指定事件，内核仅移除fd的监听
        (void) n;
    }

    //3.修改fd的事件监听状态 ---> 调用辅助函数，执行EPOLL_CTL_MOD操作
    void ModEvent(int sockfd, uint32_t events)
    {
        ModEventHelper(sockfd, events, EPOLL_CTL_MOD);
    }

    //4.等待epoll事件就绪（核心：epoll_wait封装）
    int WaitEvents(struct epoll_event revs[], int maxnum, int timeout)
    {
        //1.调用epoll_wait：阻塞等待事件就绪，就绪事件存入revs数组
        int n = epoll_wait(_epfd, revs, maxnum, timeout);
        
        //2.处理epoll_wait返回结果
        //情况一：调用失败（如被信号中断）
        if (n < 0) 
        {
            LOG(LogLevel::WARNING) << "epoll_wait error";
        }

        //情况二：超时（无事件就绪）
        else if (n == 0)  
        {
            LOG(LogLevel::WARNING) << "epoll_wait timeout";
        }

        //情况三：成功返回就绪事件数（n>0）
        else 
        {
            // TODO：可添加就绪事件数日志、事件类型解析等扩展逻辑
        }

        //3.返回就绪事件数，供Reactor的Dispatcher处理
        return n;  
    }
};
