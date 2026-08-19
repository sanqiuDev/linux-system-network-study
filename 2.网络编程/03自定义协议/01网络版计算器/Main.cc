#include <memory>        // 包含“智能指针头文件” ---> 使用std::unique_ptr管理对象，避免内存泄漏

#include "TcpServer.hpp" // 包含“TCP服务器类头文件” ---> 封装服务器的启动、连接管理、请求分发等核心功能
#include "Protocol.hpp"  // 包含“协议处理类头文件” ---> 封装客户端请求的解析、业务处理、响应构建等逻辑
#include "NetCal.hpp"    // 包含“计算器业务逻辑类”
#include "Daemon.hpp"    // 包含“进程守护化的方法”

//1.用法提示函数 ---> 当命令行参数输入错误时，打印正确的使用方式
void Usage(std::string proc)
{ 
    std::cerr << "Usage: " << proc << " port" << std::endl; 
}

//2.程序运行入口 ---> 命令行输入格式为 "./tcpserver 端口号"（例如 ./tcpserver 8080）
int main(int argc, char *argv[])
{
    /*----------------------------------------------准备阶段----------------------------------------------*/
    //1.“检查命令行参数个数” ---> 必须传入1个端口号参数（程序名+端口号共2个参数）
    if (argc != 2)
    {
        Usage(argv[0]); // 参数错误时，调用Usage函数打印用法提示
        exit(USAGE_ERR); // 退出程序，
    }

    //2.将当前进程转换为守护进程（后台运行，脱离终端控制）
    std::cout << "服务器已经启动，已经是一个守护进程了" << std::endl;
    Daemon(0, 0); // 使用自己造的轮子
    // daemon(0, 0); // 使用库提供的方法

    //3.日志策略配置 ---> 选择日志输出方式
    // Enable_Console_Log_Strategy(); // 控制台日志（守护进程模式下通常不用）
    Enable_File_Log_Strategy();       // 文件日志（守护进程需将日志写入文件）

    /*----------------------------------------------准备阶段----------------------------------------------*/
    /* ========================== 【应用层】：创建计算器对象（核心运算逻辑） ========================== */
    //1.使用unique_ptr管理Cal对象（独占所有权，自动释放内存）
    std::unique_ptr<Cal> cal = std::make_unique<Cal>();

    /* ========================== 【表示层】：创建协议处理对象（封装通信协议） ========================== */
    //2.“创建协议处理对象” ---> 使用std::unique_ptr智能指针管理，自动释放资源
    std::unique_ptr<Protocol> protocol = std::make_unique<Protocol>(
        [&cal](Request &req)->Response{
        return cal->Execute(req);  // 将业务逻辑注入协议层（解耦协议与业务）
    });


    /* ========================== 【会话层】：创建TCP服务器对象（监听端口+处理连接） ========================== */
    //3.“创建TCP服务器对象”
    std::unique_ptr<TcpServer> tsvr = std::make_unique<TcpServer>(
        /*介绍说明：使用std::unique_ptr管理TcpServer，确保服务器资源自动释放
        *    构造参数1：将命令行传入的端口号字符串转换为整数（服务器监听端口）
        *    构造参数2：lambda表达式作为请求处理回调函数，服务器收到客户端连接后自动调用
        */

        //1.转换端口号为整数
        std::stoi(argv[1]), 

        //2.匿名回调函数：绑定协议处理逻辑，参数由服务器框架传入
        [&protocol](std::shared_ptr<Socket> &sock, InetAddr &client){ 
            protocol->GetRequest(sock, client);
            /*匿名回调函数说明：
            *     1. sock：客户端连接的Socket对象（智能指针，管理通信句柄） 
            *     2. client：客户端地址对象（包含IP和端口，标识请求来源）
            *   调用协议处理对象的GetRequest方法，处理客户端请求
            * 职责链：服务器框架负责网络通信，Protocol负责业务逻辑，解耦网络层与业务层
            */
        }
    );

    //4.“启动TCP服务器” ---> 进入事件循环，持续监听端口、接收连接、分发请求
    tsvr->Start(); //注意：该方法会阻塞主线程，直到服务器主动停止或异常退出

    //5.程序正常退出
    return 0;
}