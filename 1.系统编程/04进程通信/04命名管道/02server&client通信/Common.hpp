#pragma once  

/*---------------------------------------头文件---------------------------------------*/
#include <iostream>    // 提供“输入输出流操作” ---> 如cout、cin
#include <cstdio>      // 提供“标准I/O函数” -----> 如perror
#include <string>      // 提供“string类支持”

#include <sys/types.h> // 提供“基本数据类型定义” ---> 如pid_t
#include <sys/stat.h>  // 提供“文件状态相关定义” ---> 如mkfifo函数、文件权限
#include <fcntl.h>     // 提供“文件打开相关定义” ---> 如O_RDONLY、O_WRONLY
#include <unistd.h>    // 提供“POSIX系统调用” -----> 如read、write、close、unlink

/*---------------------------------------宏定义---------------------------------------*/
//1.宏定义：管道文件的路径和名称
#define PATH "."        // 管道文件所在目录（当前目录）
#define FILENAME "fifo" // 管道文件名称

//2.错误处理宏：打印错误信息并退出程序
#define ERR_EXIT(m)         \
    do                      \
    {                       \
        perror(m);          \
        exit(EXIT_FAILURE); \
    } while (0)    // perror打印错误原因，exit终止程序


/*---------------------------------------命名管道（FIFO）管理类---------------------------------------*/
class NamedFifo 
{
private:
    //1.管道文件所在目录路径
    //2.管道文件名称
    //3.完整的管道文件路径（路径+名称）

    std::string _path;    
    std::string _name;     
    std::string _fifoname; 

public:
    //1.“构造函数” ---> 初始化管道路径和名称，并创建命名管道
    NamedFifo(const std::string &path, const std::string &name)
        : _path(path), _name(name)  
    {
        //1.拼接完整的管道文件路径（路径+文件名）
        _fifoname = _path + "/" + _name;
        
        //2.重置文件创建掩码 ---> 确保管道权限不受默认umask影响
        umask(0);
        
        //3.创建命名管道，权限为0666
        int n = mkfifo(_fifoname.c_str(), 0666);
        //情况一：创建失败
        if (n < 0)  
        {
            ERR_EXIT("mkfifo");  // 调用错误处理宏，打印错误并退出
        }
        //情况二：创建成功
        else   
        {
            std::cout << "mkfifo success" << std::endl;
        }
    }

    //2.“析构函数” ---> 删除命名管道文件
    ~NamedFifo()
    {
        //1.删除文件系统中的管道文件（内核缓冲区会自动释放）
        int n = unlink(_fifoname.c_str());

        //情况一： 删除成功
        if (n == 0)  
        {
            std::cout << "remove fifo success" << std::endl;
        }
        //情况二：删除失败
        else  
        {
             ERR_EXIT("unlink");  // 调用错误处理宏，打印错误并退出
        }
    }
};

/* 注意这里的细节：
*      1. 就是将“命名管道管理类”和“文件操作类”进行分离实现
*      2. 命名管道管理类专注于管道文件的创建与销毁，而文件操作类则负责管道的打开、读写等通信逻辑
*      3. 这种分离让代码的职责更清晰，也便于维护和扩展
*      2. 这是因为只有服务端要创建命令管道，客户端不需要创建管道文件
*/

/*---------------------------------------文件操作类---------------------------------------*/
class FileOperator
{
private:
    std::string _path;     // 管道文件所在目录路径
    std::string _name;     // 管道文件名称
    std::string _fifoname; // 完整的管道文件路径（路径+名称）

    int _fd;               // 管道文件描述符（-1表示未打开）

public:
    //1.“构造函数” ---> 初始化管道路径和名称，默认文件描述符为-1（未打开）
    FileOperator(const std::string &path, const std::string &name)
        : _path(path), _name(name),
         _fd(-1)  
    {
        _fifoname = _path + "/" + _name;
    }

    //2.“析构函数” ---> 默认不做操作，关闭文件描述符由显式调用Close()处理
    ~FileOperator()
    {
    }
 
    //3.“以只读方式打开管道” ---> 若写端未打开，读端的open会阻塞，直到写端打开后才返回
    void OpenForRead()
    {
        //1.打开管道文件，O_RDONLY表示只读模式
        _fd = open(_fifoname.c_str(), O_RDONLY);

        //情况一：打开管道失败
        if (_fd < 0)  
        {
            ERR_EXIT("open");  // 调用错误处理宏
        }
        //情况二：打开管道成功
        std::cout << "open fifo success (read mode)" << std::endl;
    }

    //4.“以只写方式打开管道” ---> 若读端未打开，写端的open会阻塞，直到读端打开后才返回
    void OpenForWrite()
    {
        //1.打开管道文件，O_WRONLY表示只写模式
        _fd = open(_fifoname.c_str(), O_WRONLY);

        //情况一：打开管道失败
        if (_fd < 0)  
        {
            ERR_EXIT("open");  // 调用错误处理宏
        }
        //情况二：打开管道成功
        std::cout << "open fifo success (write mode)" << std::endl;
    }


    //5.“向管道写入数据” ---> 循环读取用户输入，并附加序号和进程ID后写入管道
    void Write()
    {
        //1.存储用户输入的消息
        std::string message;  

        //2.消息计数器（记录第几条消息）
        int cnt = 1;        

        //3.获取当前进程ID（用于标识消息来源）
        pid_t id = getpid();  

        //4.无限循环，持续写入数据
        while (true)  
        {
            //4.5：提示用户输入
            std::cout << "Please Enter# ";  
            //4.6：读取用户输入的一行数据
            std::getline(std::cin, message);  

            //4.7：为消息附加序号和进程ID（便于接收方识别）
            message += (", message number: " + std::to_string(cnt++) + ", [" + std::to_string(id) + "]");

            //4.8：向管道写入数据（_fd为写端文件描述符）
            write(_fd, message.c_str(), message.size());
        }
    }


    //6.“从管道读取数据” ---> 循环读取管道内容，并打印到终端
    void Read()
    {
        //1.无限循环，持续读取数据
        while (true) 
        {
            //1.1： 缓冲区，用于存储读取到的数据
            char buffer[1024];  
            //1.2：从管道读取数据，最多读取sizeof(buffer)-1字节（预留1字节给'\0'）
            int number = read(_fd, buffer, sizeof(buffer) - 1);

            //情况一：读取成功（获取到数据）
            if (number > 0)  
            {
                buffer[number] = 0;  //注意：手动添加字符串结束符（确保打印正确）
                std::cout << "Client Say# " << buffer << std::endl;  
            }
            //情况二：读取到0字节（写端关闭，通信结束）
            else if (number == 0)
            {
                std::cout << "client quit! me too!" << std::endl;
                break;
            }
            //情况三： 读取错误（如被信号中断等）
            else  
            {
                std::cerr << "read error" << std::endl;
                break;  
            }
        }
    }

    //7.“关闭管道文件描述符” ---> 释放资源，避免文件描述符泄漏
    void Close()
    {
        //1.确保文件描述符有效（已打开）
        if (_fd > 0)  
        {
            close(_fd);
        }  
    }
};


