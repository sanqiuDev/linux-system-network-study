#include "server.hpp"  

// 定义MySQL数据库连接配置常量，统一管理数据库连接参数，与工具类中的配置对应
#define HOST "127.0.0.1"    // 数据库服务器地址（本地回环地址）
#define PORT 3306           // MySQL默认服务端口号
#define USER "root"         // 数据库登录用户名
#define PASS "123456"       // 数据库登录密码
#define DBNAME "gobang"     // 五子棋项目对应的目标数据库名

/**
 * @brief 主函数，程序入口
 * 功能：初始化五子棋服务器并启动服务，监听指定端口的客户端连接
 * 流程：创建gobang_server对象→调用start方法启动服务器→阻塞等待客户端连接
 */
int main()
{
    //1.实例化gobang_server对象（五子棋服务器核心类），传入数据库连接配置，初始化服务器
    gobang_server _server(HOST, USER, PASS, DBNAME, PORT);
    
    //2.调用gobang_server的start方法，启动服务器，监听8085端口的客户端WebSocket连接
    _server.start(8085);  // 该方法通常为阻塞方法，启动后会持续运行，处理客户端的连接、消息等请求
    
    //3.程序正常退出（实际中服务器启动后会阻塞，该语句通常不会执行）
    return 0;
}


