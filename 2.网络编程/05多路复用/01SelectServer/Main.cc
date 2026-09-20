#include "SelectServer.hpp"  

int main(int argc, char *argv[])
{
    //1.检查命令行参数合法性：必须传入1个端口参数（argc=2）
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " port" << std::endl;
        exit(USAGE_ERR);
    }

    //2.初始化日志策略：启用控制台日志输出
    Enable_Console_Log_Strategy();

    //3.解析端口参数：将字符串类型的端口转为无符号16位整数（符合TCP端口范围0~65535）
    uint16_t port = std::stoi(argv[1]);

    //4.创建SelectServer对象并托管给智能指针
    std::unique_ptr<SelectServer> svr = std::make_unique<SelectServer>(port);

    //5.启动服务器：进入select事件循环，持续监听客户端连接和数据读写事件
    svr->Start();

    return 0;
}