#pragma once 

#include <iostream>               
#include <string>                
#include <memory>      
#include <sys/types.h>            // 系统类型定义（socket相关）
#include <sys/socket.h>           // socket系统调用（recv/send）        
#include <functional>             // 函数对象（回调函数类型依赖）
#include <cerrno>                 // 错误码定义（errno、EAGAIN等）

#include "Common.hpp"             // 通用常量/工具（如SetNonBlock非阻塞设置）
#include "Connection.hpp"         // 连接基类（定义Recver/Sender等接口）
#include "Log.hpp"               
#include "InetAddr.hpp"          
using namespace LogModule;       

#define SIZE 1024  // 单次recv的缓冲区大小（1KB），平衡内存占用与IO效率

// Channel类：客户端连接的具体实现类
class Channel : public Connection
{
private:
    //1.客户端fd（核心标识）
    //2.输入缓冲区：存储客户端发送的原始字节流（处理粘包）
    //3.输出缓冲区：存储待发送给客户端的响应数据
    //4.客户端地址信息（IP+端口，便于日志/业务识别）

    int _sockfd;            
    std::string _inbuffer;   
    std::string _outbuffer; 
    InetAddr _client_addr;  

    // handler_t _handler;     // 注：_handler继承自Connection基类，无需重复定义


public:
    /*-----------------------------------------【构造&析构】-----------------------------------------*/
    //1.“构造函数” ---> 初始化客户端连接
    Channel(int sockfd, const InetAddr &client) : _sockfd(sockfd), _client_addr(client)
    {
        // 将客户端fd设置为非阻塞模式：适配EPOLLET（边缘触发），避免IO阻塞
        SetNonBlock(_sockfd);
    }

    //2.“析构函数” ---> 智能指针自动管理资源，无需手动清理fd（由Reactor的DelConnection关闭）
    ~Channel()
    { }

    /*-----------------------------------------【缓冲区】-----------------------------------------*/
    //1.获取输入缓冲区（供业务回调访问，如Protocol解析数据）
    std::string &Inbuffer()
    {
        return _inbuffer;
    }

    //2.追加数据到输出缓冲区（供外部手动添加响应数据）
    void AppendOutBuffer(const std::string &out)
    {
        _outbuffer += out;
    }


    /*-----------------------------------------【重写函数】-----------------------------------------*/
    //1.处理客户端fd的读事件（读取客户端发送的数据）
    void Recver() override
    {
        //1.单次读取的临时缓冲区
        char buffer[SIZE]; 

        //2.循环读取数据：EPOLLET模式下需一次性读完所有就绪数据
        while (true)
        {
            //2.1：清空临时缓冲区，避免脏数据
            buffer[0] = 0;  

            //2.2：非阻塞recv：从客户端fd读取数据，flags=0（默认）
            ssize_t n = recv(_sockfd, buffer, sizeof(buffer) - 1, 0);

            //情况一：成功读取到数据
            if (n > 0)  
            {
                //1）字符串末尾加结束符
                buffer[n] = 0;                

                //2）将数据追加到输入缓冲区（处理粘包的基础）
                _inbuffer += buffer;         
            }

            //情况二：客户端关闭连接（FIN包）
            else if (n == 0)  
            {
                Excepter();  
                return;
            }

            //情况三：recv失败（n < 0）
            else  
            {
                //1）无数据可读（非阻塞模式下的正常情况），退出读取循环
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                { 
                    break;
                }

                //2）被信号中断，继续循环读取
                else if (errno == EINTR)
                {
                    continue;
                }

                //3）其他错误（如fd错误），调用异常处理
                else
                {
                    Excepter();
                    return;
                }
            }
        }

        //3.调试日志：打印输入缓冲区内容，便于排查数据解析问题
        LOG(LogLevel::DEBUG) << "Channel: Inbuffer:\n" << _inbuffer;

        //4.输入缓冲区非空 ---> 调用业务回调处理数据
        if (!_inbuffer.empty())
        {  
            _outbuffer += _handler(_inbuffer); // 处理后的数据追加到输出缓冲区，等待发送
        }

        //5.输出缓冲区非空 ---> 触发写事件发送响应（最佳实践：读取完数据立即尝试发送）
        if (!_outbuffer.empty())
        {
            Sender();  // 调用写处理函数

            // 备选方案：开启写事件监听，由Reactor触发Sender（适合大流量场景）
            // GetOwner()->EnableReadWrite(_sockfd, true, true);
        }
    }

    //2.处理客户端fd的写事件（发送响应数据给客户端）
    void Sender() override
    {
        //1.循环发送数据：EPOLLET模式下需一次性发送所有就绪数据
        while (true)
        {
            //1.1：非阻塞send发送输出缓冲区数据
            ssize_t n = send(_sockfd, _outbuffer.c_str(), _outbuffer.size(), 0);

            //情况一：成功发送部分/全部数据
            if (n > 0)  
            {
                //1）从输出缓冲区删除已发送的字节（n为发送成功的字节数）
                _outbuffer.erase(0, n);

                //2）输出缓冲区为空：所有数据发送完毕，退出循环
                if (_outbuffer.empty())
                {
                    break;
                }
            }

            //情况二：客户端关闭连接，退出循环
            else if (n == 0)  
            {
                break;
            }

            //情况二：send失败（n < 0）
            else  
            {
                //1）发送缓冲区满（非阻塞模式下的正常情况），退出循环
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }

                //2）被信号中断，继续循环发送
                if (errno == EINTR)
                {
                    continue;
                }

                //3）其他错误，调用异常处理
                else
                {
                    Excepter();
                    return;
                }
            }
        }

        //1.2：调整写事件监听状态
        //情况一：输出缓冲区非空：发送未完成，开启写事件监听，等待下次可写
        if (!_outbuffer.empty())
        {
            GetOwner()->EnableReadWrite(_sockfd, true, true);  // 开启读+写事件
        }

        //情况二：输出缓冲区为空：发送完成，关闭写事件监听，避免无效触发
        else
        {
            GetOwner()->EnableReadWrite(_sockfd, true, false); // 仅保留读事件
        }
    }

    //3.统一处理所有IO异常（错误/关闭/挂起）---> 通知Reactor删除当前连接，释放fd和资源
    void Excepter() override
    {
        GetOwner()->DelConnection(_sockfd);
    }

    //4.返回客户端fd，供Reactor/Epoller调用
    int GetSockFd() override
    {
        return _sockfd;
    }
};