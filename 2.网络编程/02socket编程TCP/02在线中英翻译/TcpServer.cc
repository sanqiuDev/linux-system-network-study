#include "TcpServer.hpp" // 包含 ---> TCP服务器封装类头文件（封装了TCP服务器的初始化、启动、连接处理等核心逻辑）
#include "Dict.hpp"      // 包含 ---> 翻译模块头文件（预留扩展，如后续需实现翻译功能可取消注释）

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
    //1.创建翻译字典对象
    Dict d;          

    //2.加载字典数据（如从文件读取单词-翻译映射关系）
    d.LoadDict();    

    /* ================================= TCP服务器创建与配置 ================================= */
    //1.翻译功能的回调绑定
    std::unique_ptr<TcpServer> tsvr =
        std::make_unique<TcpServer>(
            port,
            [&d](const std::string &word, InetAddr &addr)
            {
                return d.Translate(word, addr);  // 客户端发送单词，服务器返回翻译结果
            });

    /* ================================= 启动TCP服务器 ================================= */
    //1.服务器初始化：创建监听套接字、绑定端口、设置监听（底层调用socket、bind、listen）
    tsvr->Init();  
    
    //2.服务器启动：进入事件循环（持续监听客户端连接，接收请求并调用回调函数处理，永不返回直到进程终止）
    tsvr->Run();   

    return 0;
}