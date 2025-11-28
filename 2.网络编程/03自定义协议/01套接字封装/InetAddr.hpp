#pragma once  
#include "Common.hpp"  // 包含 ---> 通用工具头文件（提供sockaddr_in结构体、CONV宏、系统头文件依赖等）

// 网络地址封装类 ---> 封装IPv4地址（struct sockaddr_in）的操作，
class InetAddr 
{
private:
    //1.存储网络字节序的IPv4地址和端口（底层依赖）
    //2.主机字节序的IP
    //3.主机字节序的端口号

    struct sockaddr_in _addr; 
    std::string _ip;         
    uint16_t _port;           

public:
    /*------------------------------------------【构造析构】------------------------------------------*/
    //1.默认构造函数 ---> 创建空的地址对象（后续需手动设置IP和端口）
    InetAddr(){}

    //2.构造函数 ---> 通过已有的sockaddr_in结构体创建地址对象（网络字节序→主机字节序）
    InetAddr(struct sockaddr_in &addr) : _addr(addr)
    {
        /*----------------------- “端口的转换” -----------------------*/
        //1.网络字节序端口 → 主机字节序端口
        _port = ntohs(_addr.sin_port);  


        /*------------------------ “IP的转换” ------------------------*/
        //2.方法一：二进制网络字节序格式IP → 点分十进制字符串格式IP（老式IP转换）
        // _ip = inet_ntoa(_addr.sin_addr); 

        //2.方法二：二进制网络字节序格式IP → 点分十进制字符串格式IP（现代IP转换）
        //2.1：存储转换后的IP字符串
        char ipbuffer[64];   // 点分十进制最大长度为15，64字节足够
        //2.2：进行IP地址格式的转换
        inet_ntop(AF_INET, &_addr.sin_addr, ipbuffer, sizeof(ipbuffer));
        //2.3：将缓冲区的字符串赋值给成员变量_ip
        _ip = ipbuffer;  
    }

    //3.构造函数 ---> 通过IP字符串和端口号创建地址对象（主机字节序→网络字节序）
    InetAddr(const std::string &ip, uint16_t port) : _ip(ip), _port(port)
    {
        //1.初始化sockaddr_in结构体 ---> 将所有字节置0，避免随机垃圾值
        memset(&_addr, 0, sizeof(_addr));

        /*----------------------- “IP的转换” -----------------------*/
        //现代方法：
        //2.设置地址族为IPv4
        _addr.sin_family = AF_INET;
        
        //3.点分十进制字符串IP → 网络字节序IP
        inet_pton(AF_INET, _ip.c_str(), &_addr.sin_addr); 

        //老式方法：
        //_addr.sin_addr.s_addr = inet_addr(_ip.c_str()); 

        /*----------------------- “端口的转换” -----------------------*/
        //4.主机字节序端口 → 网络字节序端口
        _addr.sin_port = htons(_port);
    }

    //4.构造函数：仅通过端口号创建地址对象（绑定所有网卡，主机字节序→网络字节序）
    InetAddr(uint16_t port) :_port(port),_ip()  // _ip默认初始化为空字符串（后续可通过StringAddr()获取"0.0.0.0"）
    {
        //1.初始化sockaddr_in结构体：将所有字节置0
        memset(&_addr, 0, sizeof(_addr));
        
        //2.设置地址族为IPv4
        _addr.sin_family = AF_INET;
        
        //3.绑定所有本地网卡 ---> INADDR_ANY等价于0.0.0.0，系统会自动绑定所有可用的网络接口
        _addr.sin_addr.s_addr = INADDR_ANY;


        /*----------------------- “端口的转换” -----------------------*/
        //4.主机字节序端口 → 网络字节序端口
        _addr.sin_port = htons(_port);
    }

    
    //5.析构函数：空实现（无动态分配的资源需要释放）
    ~InetAddr() {}

    /*------------------------------------------【成员方法】------------------------------------------*/
    //1.获取主机字节序的端口号（对外提供统一的端口访问接口）
    uint16_t Port() { return _port; }

    //2.获取点分十进制字符串格式的IP（对外提供统一的IP访问接口）
    std::string Ip() { return _ip; }

    //3.获取网络字节序的sockaddr_in结构体（只读，用于需要完整地址结构的场景）
    const struct sockaddr_in &NetAddr() { return _addr; }


    /*-------------------------------为了bind接口的参数-------------------------------*/
    //4.获取sockaddr_in结构体的大小
    socklen_t NetAddrLen()
    {
        return sizeof(_addr); // 避免手动编写sizeof(struct sockaddr_in)，提升代码可读性
    }

    //5.获取sockaddr*类型的地址指针 ---> 通过CONV宏将sockaddr_in*转换为sockaddr*，适配API的参数要求
    const struct sockaddr *NetAddrPtr()
    {
        return CONV(_addr);  // 等价于 (struct sockaddr*)&_addr
    }




    //6.获取「IP:端口」格式的字符串 ---> 用于日志打印、调试，直观展示地址
    std::string StringAddr()
    {
        return _ip + ":" + std::to_string(_port);
    }

    //7.重载==运算符 ---> 判断两个连接是否来自同一个客户端（相同IP和端口）
    bool operator==(const InetAddr &addr)
    {
        return addr._ip == _ip && addr._port == _port; // 用于比较两个地址对象是否相等（IP和端口都相同）
    }


    //8.设置网络地址 ---> 将传入的sockaddr_in结构体转换为本地存储的主机字节序地址信息
    void SetAddr(struct sockaddr_in &addr)
    {
        //1.保存原始的网络字节序地址结构体（用于后续socket API调用）
        _addr = addr;

        //2.网络字节序端口 → 主机字节序端口
        _port = ntohs(_addr.sin_port); // 将16位网络字节序（大端）转换为主机字节序（小端）

        //3.网络字节序IP地址 → 点分十进制字符串IP（主机字节序）
        //3.1：临时缓冲区，存储转换后的IP字符串
        char ipbuffer[64]; 
        //3.2：使用函数inet_ntop进行转换
        inet_ntop(AF_INET, &_addr.sin_addr, ipbuffer, sizeof(ipbuffer));

        //4.保存转换后的点分十进制IP字符串（主机字节序，便于对外展示和使用）
        _ip = ipbuffer;
    }
};