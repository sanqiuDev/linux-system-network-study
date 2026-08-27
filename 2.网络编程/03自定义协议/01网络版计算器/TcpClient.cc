#include <iostream>       // 标准输入输出（打印提示信息）
#include <string>         // 字符串处理（存储服务器IP）
#include <memory>         // 智能指针（管理Socket对象生命周期）

#include "Socket.hpp"     // Socket模块头文件（提供TCP Socket通信能力）
#include "Common.hpp"     // 公共常量/枚举（如错误码USAGE_ERR）
#include "Protocol.hpp"   // 协议处理类（封装请求构建、响应解析逻辑）

//1.引入Socket模块命名空间，简化Socket相关类的使用
using namespace SocketModule; 


/*---------------------------------------------辅助函数--------------------------------------------*/
//1.打印程序使用方法
void Usage(std::string proc)
{
    std::cerr << "Usage: " << proc << " server_ip server_port" << std::endl;
}


//2.从标准输入读取用户输入的运算参数
void GetDataFromStdin(int *x, int *y, char *oper)
{
    std::cout << "Please Enter x: ";  
    std::cin >> *x;                   

    std::cout << "Please Enter y: ";  
    std::cin >> *y;          

    std::cout << "Please Enter oper: "; 
    std::cin >> *oper;                
}


/*---------------------------------------------主函数--------------------------------------------*/
//1.TCP客户端程序入口 ---> 命令行参数：./tcpclient [服务器IP] [服务器端口]
int main(int argc, char *argv[])
{
    /*==============================第一步：准备阶段==============================*/
    //1.检查命令行参数 ---> 必须传入服务器IP和端口（argc=3）
    if (argc != 3)
    {
        Usage(argv[0]);       
        exit(USAGE_ERR);    
    }


    /*==============================第二步：连接服务器==============================*/
    //1.解析命令行参数 ---> 提取服务器IP和端口
    std::string server_ip = argv[1];          
    uint16_t server_port = std::stoi(argv[2]);

    //2.创建TCP客户端Socket对象（使用unique_ptr管理，自动释放内存）
    std::shared_ptr<Socket> client = std::make_unique<TcpSocket>();
    //3.构建TCP客户端Socket（仅创建Socket文件描述符，无需绑定/监听）
    client->BuildTcpClientSocketMethod();

    //4.连接TCP服务器 ---> 调用Socket的Connect方法
    if (client->Connect(server_ip, server_port) != 0)
    {
        std::cerr << "connect error" << std::endl;
        exit(CONNECT_ERR);  
    }

    /*==============================第三步：交互式请求循环==============================*/
    //1.创建协议处理对象（封装请求构建、响应解析逻辑）
    std::unique_ptr<Protocol> protocol = std::make_unique<Protocol>();

    //2.定义响应缓冲区 ---> 存储服务器返回的原始数据（处理半包/粘包）
    std::string resp_buffer; 

    //3.连接服务器成功 ---> 进入交互式请求循环
    while (true)
    {
        //3.1：从标准输入读取用户输入的运算参数
        int x, y;
        char oper;
        GetDataFromStdin(&x, &y, &oper);

        //3.2：构建请求 ---> 将运算参数封装为协议格式的字符串（JSON+长度前缀）
        std::string req_str = protocol->BuildRequestString(x, y, oper);
        
        //3.3：发送请求 ---> 将协议格式的请求字符串发送给服务器
        client->Send(req_str);




        //3.4：解析响应 ---> 从服务器接收数据，提取完整响应并反序列化为Response对象
        Response resp;  
        bool res = protocol->GetResponse(client, resp_buffer, &resp);
        if(res == false) 
            break;

        //3.5：显示结果 ---> 打印运算结果或错误信息
        resp.ShowResult();
    }

    //4.关闭Socket连接，释放资源
    client->Close();
    return 0;
}