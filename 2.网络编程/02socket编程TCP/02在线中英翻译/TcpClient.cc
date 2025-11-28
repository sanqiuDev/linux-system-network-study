#include <iostream>
#include "Common.hpp"    // 包含“通用工具头文件” ---> 如错误码定义、常用宏等，用户自定义
#include "InetAddr.hpp"  // 包含“网络地址封装类头文件” ---> 如封装sockaddr_in结构，简化地址操作


//1.“用法提示函数” ---> 当用户输入参数错误时，打印正确的命令行用法
void Usage(std::string proc) // 参数：proc - 程序名（argv[0]传入，即当前可执行文件名）
{
    //1.标准错误流输出用法提示
    std::cerr << "Usage: " << proc << " server_ip server_port" << std::endl; //std::cerr默认不缓冲，直接输出错误信息
}


//2.“程序入口” ---> TCP客户端主逻辑（命令行参数格式：./tcpclient 服务器IP 服务器端口）
/* main函数的参数说明：
*      1. argc：命令行参数个数
*      2. argv：参数数组
*           2.1：argv[0]是程序名
*           2.2：argv[1]是服务器IP
*           2.3：argv[2]是服务器端口
*/
int main(int argc, char *argv[])
{
    //1.检查命令行参数个数 ---> 必须传入服务器IP和端口，共3个参数（含程序名）
    if(argc != 3)
    {
        Usage(argv[0]);   // 参数错误，调用Usage函数打印用法提示
        exit(USAGE_ERR);  // 退出程序，USAGE_ERR是Common.hpp中定义的参数错误码
    }

    //2.解析命令行参数 ---> 将字符串类型的IP和端口转换为程序可用的格式
    std::string serverip = argv[1];           // 服务器IP地址（如"127.0.0.1"）         
    uint16_t serverport = std::stoi(argv[2]); // 服务器端口号（将字符串转换为16位无符号整数，范围0-65535）

    /* ========================= 步骤1：创建TCP套接字（Socket）========================= */
    //3.socket函数 ---> 创建用于网络通信的文件描述符（套接字）
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) 
    {
        std::cerr << "socket error" << std::endl;  // 输出创建失败信息
        exit(SOCKET_ERR);  // 退出程序，SOCKET_ERR是Common.hpp中定义的套接字创建错误码
    }

    /*关键说明：客户端是否需要bind？
    *  注意：不需要显式调用bind()！
    *  原因：
    *    bind()的作用是将“套接字”与“固定的本地IP和端口”绑定。
    *    客户端的核心需求是"连接服务器"，而非让服务器主动连接自己，因此：
    *        1. 本地IP：系统会自动选择当前网络接口的IP（如：WiFi、以太网对应的IP）
    *        2. 本地端口：系统会从"临时端口池"（1024-65535）中随机分配一个未使用的端口
    *    显式bind会增加端口冲突风险（若指定的端口已被占用），完全没必要。
    * 
    * ------------------------------------------------------------------------------
    * 
    * 注意：不需要调用listen()和accept()！
    * 原因：
    *   这两个函数是服务器端专用的：
    *       1. listen用于监听端口
    *       2. accept用于接收客户端连接 
    *   客户端是"主动发起连接"的一方，只需发起连接请求即可
    */ 

    /* ========================= 步骤2：向服务器发起TCP连接请求 ========================= */
    //4.初始化服务器地址 ---> 使用InetAddr类封装sockaddr_in结构（简化地址赋值）
    InetAddr serveraddr(serverip, serverport); // 传入服务器IP和端口，内部自动转换为网络字节序

    //5.调用connect函数发起连接
    int n = connect(sockfd, serveraddr.NetAddrPtr(), serveraddr.NetAddrLen());
    if(n < 0) 
    {
        std::cerr << "connect error" << std::endl;  // 输出连接失败信息（可能原因：服务器未启动、IP/端口错误、网络不通）
        exit(CONNECT_ERR);  // 退出程序，CONNECT_ERR是Common.hpp中定义的连接错误码
    }

    /* ========================= 步骤3：TcpEchoServer客户端核心逻辑========================= */
    //6.持续向服务器发送数据，并接收服务器的响应
    while(true)
    {
        //1.读取用户输入：从标准输入获取一行字符串（要发送给服务器的数据）
        std::string line;              // 存储用户输入的字符串
        std::cout << "Please Enter@ "; // 输入提示（@标识客户端输入）
        std::getline(std::cin, line);  // 读取一行输入（包含空格，区别于std::cin >> line）

        //2.发送数据到服务器：通过套接字写入数据
        write(sockfd, line.c_str(), line.size());
        /* write函数：将数据从应用层写入内核的TCP发送缓冲区
        *      1. 参数1：sockfd - 连接成功的套接字描述符
        *      2. 参数2：line.c_str() - 字符串的首地址（转换为const void*）
        *      3. 参数3：line.size() - 要发送的字节数（字符串长度，不含末尾的'\0'） 
        */

        //3.接收服务器的响应：从套接字读取数据
        char buffer[1024]; 
        ssize_t size = read(sockfd, buffer, sizeof(buffer)-1);
        /* read函数：从内核的TCP接收缓冲区读取数据到应用层
        *     1. 参数1：sockfd - 连接成功的套接字描述符
        *     2. 参数2：buffer - 接收缓冲区的首地址
        *     3. 参数3：sizeof(buffer)-1 - 最大读取字节数（留1字节给字符串结束符'\0'）
        */
        if(size > 0)  
        {
            buffer[size] = 0;   // 在接收数据末尾添加字符串结束符，避免打印乱码
            std::cout << "server echo# " << buffer << std::endl;  // 打印服务器的响应数据（#标识服务器响应）
        }
    }

    /* ========================= 步骤4：关闭套接字（释放资源）========================= */
    //7.关闭套接字描述符，释放对应的内核资源（TCP连接会优雅关闭）
    close(sockfd); 
    return 0;
}
