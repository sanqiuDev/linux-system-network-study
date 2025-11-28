#pragma once  

#include <iostream>         // 标准输入输出（预留调试使用）
#include <string>           // 字符串处理（序列化/反序列化的载体）
#include <memory>           // 智能指针（用于Socket对象管理）
#include "Socket.hpp"       // Socket模块头文件（提供Socket通信能力）

// 引入Socket模块命名空间，简化Socket相关类的使用
using namespace SocketModule;

// 实现一个自定义的网络版本计算器协议栈：
// 包含请求(Request)、响应(Response)、协议处理(Protocol)三层，
// 核心解决「数据序列化/反序列化」和「TCP粘包/半包」问题

// 约定好各个字段的含义，本质就是约定好协议！
// client -> server
// 如何要做序列化和反序列化：
// 1. 我们自己写(怎么做) ---> 往往不具备很好的扩展性
// 2. 使用现成的方案(这个是我们要写的) ---> json -> jsoncpp



/*-------------------------------------------------【请求类】-------------------------------------------------*/
// 请求类 ---> 封装客户端发送给服务器的计算请求（如"10 + 20"）
class Request
{
private:
    int _x;     // 第一个运算数
    int _y;     // 第二个运算数
    char _oper; // 运算符（+ - * / %），表示 _x _oper _y 的运算逻辑

public:
    //1.“默认构造函数” ---> 创建空请求对象（用于反序列化时初始化）
    Request(){}

    //2.“带参构造函数” ---> 通过具体的计算参数创建请求对象
    Request(int x, int y, char oper)
     : _x(x),
      _y(y), 
      _oper(oper)
    { }

    //3.“析构函数” ---> 空实现（无动态分配资源）
    ~Request() {}


    //4.“序列化” ---> 将请求对象转换为可网络传输的字符串
    std::string Serialize()
    {
        //1.示例序列化逻辑：用空格分隔字段，如"10 20 +"
        return std::to_string(_x) + " " + std::to_string(_y) + " " + _oper;
    }

    //5.“反序列化” ---> 将网络接收的字符串还原为请求对象
    bool Deserialize(std::string &in)
    {
        //1.示例反序列化逻辑：按空格分割字符串，解析出_x、_y、_oper
        size_t pos1 = in.find(' ');
        size_t pos2 = in.find(' ', pos1 + 1);
        if (pos1 == std::string::npos || pos2 == std::string::npos)
        {
           return false;
        }

        _x = std::stoi(in.substr(0, pos1));
        _y = std::stoi(in.substr(pos1 + 1, pos2 - pos1 - 1));
        _oper = in[pos2 + 1];
        return true;
    }

};


/*-------------------------------------------------【响应类】-------------------------------------------------*/
// 响应类 ---> 封装服务器返回给客户端的计算结果
class Response
{
private:
    //1.运算结果 ---> 需配合_code判断是否为有效结果
    //2.状态码 ---> 0=成功，1=除零错误，2=无效运算符，3=数值溢出等
    int _result; 
    int _code;   

public:
    //1.“默认构造函数” ---> 创建空响应对象（用于反序列化时初始化）
    Response() {}

    //2.“带参构造函数” ---> 通过计算结果和状态码创建响应对象
    Response(int result, int code)
    : _result(result),
     _code(code)
    {}

    //3.“析构函数” ---> 空实现
    ~Response() {}

    //4.“序列化” ---> 将响应对象转换为可网络传输的字符串
    std::string Serialize()
    {
        //1.示例序列化逻辑：用空格分隔结果和状态码，如"30 0"
        return std::to_string(_result) + " " + std::to_string(_code);
    }

    //5.“反序列化：” ---> 将网络接收的字符串还原为响应对象
    bool Deserialize(std::string &in)
    {
        //1.示例反序列化逻辑：按空格分割字符串，解析出_result和_code
        size_t pos = in.find(' ');
        if (pos == std::string::npos)
            return false;

        _result = std::stoi(in.substr(0, pos));
        _code = std::stoi(in.substr(pos + 1));
        return true;
    }
};



// 协议(基于TCP的)需要解决两个问题：
// 1. request和response必须得有序列化和反序列化功能（对象与字节流的转换）
// 2. 你必须保证，读取的时候，读到完整的请求(TCP是流式协议，可能出现粘包/半包，UDP是数据报，无需考虑)

/*-------------------------------------------------【协议处理类】-------------------------------------------------*/
// 协议处理类 ---> 封装完整的TCP通信协议逻辑（请求接收、解析、处理、响应发送）
class Protocol
{
private:
    // 因为我们用的是多进程模型：每个子进程独立处理一个客户端请求，
    // 所以不需要类成员存储Request/Response（避免多进程间资源竞争），
    // 而是在GetRequest函数内创建局部对象即可
    Request _req;
    Response _resp;

public:
    //1.“构造函数” ---> 初始化协议对象
    Protocol()
    {}

    //2.“析构函数” ---> 空实现
    ~Protocol()
    {}

    //3.“获取并处理客户端请求” ---> 完整的协议交互流程
    void GetRequest(std::shared_ptr<Socket> &sock, InetAddr &client)
    {
        // 完整协议逻辑示例：
        // 1. 从Socket读取数据（需处理粘包/半包，确保读到完整请求）
        // 2. 调用Request::Deserialize解析请求
        // 3. 根据请求计算结果（如10+20=30）
        // 4. 创建Response对象封装结果和状态码
        // 5. 调用Response::Serialize序列化响应
        // 6. 通过Socket发送响应数据
    }


};