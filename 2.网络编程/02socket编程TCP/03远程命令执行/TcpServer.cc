#include "TcpServer.hpp" // 包含 ---> TCP服务器封装类头文件（封装了TCP服务器的初始化、启动、连接处理等核心逻辑）
#include "Command.hpp"   // 包含 ---> 命令执行模块头文件（封装了系统命令执行逻辑，如Command类的Execute方法）

//1.默认请求处理函数 ---> 当未指定其他处理逻辑时使用（示例回调函数）
std::string defaulthandler(const std::string &word, InetAddr &addr)
{
    //1.打印调试日志 ---> 记录当前回调到默认处理函数（便于调试时确认执行流程）
    LOG(LogLevel::DEBUG) << "回调到了defaulthandler";

    //2.构造响应数据 ---> 在客户端请求数据前添加"haha, "前缀
    std::string s = "haha, ";
    s += word;
    return s;
}
/*defaulthandler函数的参数和返回值的说明：
*   参数1：word - 客户端发送的请求数据（字符串格式）
*   参数2：addr - 客户端的网络地址对象（包含IP和端口，可用于记录请求来源）
* 返回值：std::string - 服务器的响应数据（返回给客户端）
*/


//2.用法提示函数 ---> 当用户输入的命令行参数错误时，打印正确的使用方式
void Usage(std::string proc) // 参数：proc - 程序名（即argv[0]，当前可执行文件的名称）
{
    // 向标准错误流输出用法提示
    std::cerr << "Usage: " << proc << " port" << std::endl; // std::cerr默认不缓冲，直接显示错误信息
}


//3.程序核心功能 ---> 基于TCP的远程命令执行服务器
int main(int argc, char *argv[])
{
    /* ================================= 程序准备阶段 ================================= */
    //1.检查命令行参数个数：必须传入1个端口号参数（程序名+端口号共2个参数）
    if(argc != 2)
    {
        Usage(argv[0]);  // 参数错误时，调用Usage函数打印用法提示
        exit(USAGE_ERR); // 退出程序，USAGE_ERR是自定义的错误码（表示参数使用错误）
    }

    //2.将命令行传入的字符串类型端口号，转换为16位无符号整数（符合TCP端口的取值范围0-65535）
    uint16_t port = std::stoi(argv[1]);

    //3.启用控制台日志策略 ---> 设置日志输出到控制台（便于实时查看服务器的运行状态、调试信息）
    Enable_Console_Log_Strategy();

    /* ================================= 翻译模块初始化 ================================= */
    //1.创建Command对象（封装了系统命令执行逻辑）
    Command cmd;     

    /* ================================= TCP服务器创建与配置 ================================= */
    //1.创建TCP服务器实例，并绑定"请求处理回调函数"（服务器收到客户端数据后，自动调用该函数处理）
    std::unique_ptr<TcpServer> tsvr = 
    std::make_unique<TcpServer>( 
        //1.1：服务器监听端口（客户端需连接该端口）
        port, 

        //1.2：绑定回调函数：将Command类的Execute方法作为请求处理逻辑
        std::bind(&Command::Execute, &cmd, std::placeholders::_1, std::placeholders::_2)
        /*说明：
        *   1. std::bind：将成员函数&Command::Execute与对象&cmd绑定，固定调用者
        *   2. std::placeholders::_1、_2：占位符，对应Execute方法的两个参数（const std::string &cmd, InetAddr &addr）
        */
    );


    // //2.回调函数的另一种写法（lambda表达式，与std::bind功能等价，更简洁）
    // std::unique_ptr<TcpServer> tsvr =
    //     std::make_unique<TcpServer>(
    //         port,
    //         [&cmd](const std::string &command, InetAddr &addr)
    //         {
    //             return cmd.Execute(command, addr); // 捕获Command对象cmd，调用其Execute方法处理请求
    //         });


    /* ================================= 启动TCP服务器 ================================= */
    //1.服务器初始化：创建监听套接字、绑定端口、设置监听（底层调用socket、bind、listen）
    tsvr->Init();  
    
    //2.服务器启动：进入事件循环（持续监听客户端连接，接收请求并调用回调函数处理，永不返回直到进程终止）
    tsvr->Run();   

    return 0;
}