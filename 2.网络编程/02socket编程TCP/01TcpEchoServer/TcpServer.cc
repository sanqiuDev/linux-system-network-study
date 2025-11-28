#include "TcpServer.hpp"  // 包含TCP服务器的头文件：引入TcpServer类的定义，用于创建和启动TCP服务器

//1.用法提示函数：当用户输入的命令行参数错误时，打印正确的使用方式
void Usage(std::string proc) // 参数：proc - 程序名（即argv[0]，当前可执行文件的名称）
{
    // 向标准错误流输出用法提示
    std::cerr << "Usage: " << proc << " port" << std::endl; // std::cerr默认不缓冲，直接显示错误信息
}

//2.程序运行方式 ---> 在命令行输入 "./tcpserver 端口号"（例如 ./tcpserver 8080）
int main(int argc, char *argv[])
{
    //1.检查命令行参数个数：必须传入1个端口号参数（程序名+端口号共2个参数）
    if(argc != 2)
    {
        Usage(argv[0]); // 参数错误时，调用Usage函数打印用法提示
        exit(USAGE_ERR); // 退出程序，USAGE_ERR是自定义的错误码（表示参数使用错误）
    }

    //2.将命令行传入的字符串类型端口号，转换为16位无符号整数（符合TCP端口的取值范围0-65535）
    uint16_t port = std::stoi(argv[1]);

    //3.启用控制台日志策略 ---> 设置日志输出到控制台（便于实时查看服务器的运行状态、调试信息）
    Enable_Console_Log_Strategy();

    //4.创建TCP服务器实例
    std::unique_ptr<TcpServer> tsvr = std::make_unique<TcpServer>(port);
    /*说明：
    *   1. 使用std::unique_ptr智能指针管理TcpServer对象，自动释放资源，避免内存泄漏
    *   2. std::make_unique<TcpServer>(port)：构造TcpServer对象，传入监听端口号
    */

    //5.初始化服务器 ---> 完成socket创建、端口绑定、监听状态设置（TCP服务器启动的前三步）
    tsvr->Init();

    //6.启动服务器 ---> 进入事件循环，持续接收客户端连接并处理请求（此函数会阻塞，直到服务器被终止）
    tsvr->Run();

    return 0;
}