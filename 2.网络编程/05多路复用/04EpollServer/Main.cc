#include <iostream>       
#include <string>        

#include "Reactor.hpp"    // 反应堆模型核心类（事件循环、事件分发）
#include "Listener.hpp"   // 监听器类（封装监听套接字、新连接处理）
#include "Channel.hpp"    // 通道类（封装fd及对应的事件处理逻辑）
#include "Log.hpp"        // 日志模块（记录调试、信息、错误日志）
#include "Common.hpp"     // 通用常量/工具类（如USAGE_ERR错误码）
#include "Protocol.hpp"   // 协议类（封装请求/响应的编解码、业务执行）
#include "NetCal.hpp"     // 网络计算器业务类（Cal类，实现具体的计算逻辑）


// 打印程序使用方法：提示用户正确传入命令行参数
static void Usage(std::string proc)
{
    std::cerr << "Usage: " << proc << " port" << std::endl;
}


// 程序入口：启动基于Reactor模式的网络计算器服务器
int main(int argc, char *argv[])
{
    /*---------------------------------------------------【准备阶段】---------------------------------------------------*/
    //1.“校验命令行参数” ---> 必须传入1个端口参数（argc=2）
    if (argc != 2)
    {
        Usage(argv[0]);         
        exit(USAGE_ERR);      
    }

    //2.“初始化日志策略” ---> 启用控制台日志输出
    LogModule::ConsoleLogStrategy(); // 日志会打印到终端，便于调试服务器运行状态（如协议解析、业务执行、连接事件）

    //3.“解析端口参数” ---> 将字符串类型的端口转为无符号16位整数（符合TCP端口范围0~65535）
    uint16_t port = std::stoi(argv[1]);



    /*---------------------------------------------------【构建阶段】---------------------------------------------------*/
    //4.“构建业务模块” ---> 创建网络计算器业务对象
    std::shared_ptr<Cal> cal = std::make_shared<Cal>();     // 使用shared_ptr管理，便于多模块共享（协议对象需要调用其Execute方法）

    //5.“构建协议对象” ---> 封装请求解码、响应编码、业务执行的核心逻辑
    std::shared_ptr<Protocol> protocol = std::make_shared<Protocol>( // 构造参数为lambda表达式
        [&cal](Request &req) -> Response
        {
            return cal->Execute(req); // 协议层调用业务层逻辑：将解析后的请求交给Cal执行，返回计算结果
        });

    //6.“构建监听器对象” ---> 封装监听套接字，负责接受新客户端连接
    std::shared_ptr<Connection> conn = std::make_shared<Listener>(port);

    //7.“为监听器注册数据处理回调函数” ---> 定义接收到客户端数据后的处理逻辑
    conn->RegisterHandler([&protocol](std::string &inbuffer) -> std::string 
    { 
        LOG(LogLevel::DEBUG) << "进入到匿名函数中..."; // 调试日志：标记进入数据处理流程
        
        //1.存储最终要返回给客户端的响应数据
        std::string response_str;
        
        //2.循环解析缓冲区 ---> 处理粘包问题，确保每次解析完整的请求包
        while (true)
        {
            //2.1：存储单次解析出的完整请求包
            std::string package; 

            //2.2：调用协议层的解码方法：从原始字节流中提取完整的请求包
            if (!protocol->Decode(inbuffer, &package))
            {
                break;  // 若解码失败（缓冲区数据不足，无法组成完整包），跳出循环等待后续数据
            }

             
            //2.3：解码成功调用协议层的执行方法：处理请求并生成响应字符串
            response_str += protocol->Execute(package);  // 注意：package是一个完整的请求字节流
        }
        
        LOG(LogLevel::DEBUG) << "结束匿名函数中...: " << response_str; // 调试日志：标记数据处理完成，打印响应内容

        //3.返回响应数据，由Reactor框架发送给客户端
        return response_str; 
    });

    //8.“构建反应堆核心对象” ---> 负责事件循环、事件分发
    std::unique_ptr<Reactor> R = std::make_unique<Reactor>();

    //9.“将监听器连接添加到Reactor中” ---> Reactor开始管理监听fd的事件（如读事件=新连接到来）
    R->AddConnection(conn);



    /*---------------------------------------------------【执行阶段】---------------------------------------------------*/
    //10.“启动Reactor事件循环” ---> 阻塞在此处，持续监听并处理所有注册的fd事件
    R->Loop(); // 事件循环逻辑：等待事件就绪→分发事件→调用注册的回调函数处理

    return 0;
}