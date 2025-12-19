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
            std::cout << "remove fifo false" << std::endl;
            // ERR_EXIT("unlink");  注意：打开这行代码的话，共享内存的析构函数将不会被调用，因为服务端的进程将会在这里被终止
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

    //5.“唤醒操作” ---> 向管道写入一个字符，用于通知阻塞在读端的进程
    void Wake_Write()
    {
        //1.定义一个待写入的字符
        char c = 'c'; 

        //2.向管道写端写入1字节数据
        int n = write(_fd, &c, 1);

        //3.打印写入结果
        printf("尝试唤醒: %d\n", n); //注意：n为实际写入的字节数，若成功则为1
    }


    //6.“等待操作” ---> 从管道读取数据，若管道无数据则阻塞，直到有数据写入或写端关闭
    bool Wait_Read()
    {
        //1.定义用于存储读取到内容的字符
        char c;  

        //2.从管道读端读取1字节数据
        int n = read(_fd, &c, 1);
        if(n > 0)   // 读取成功（实际读取到字节数>0）
        {
            printf("醒来: %d\n", n);  
            return true;  
        }

        //3.读取失败或写端关闭，返回false
        return false;  
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


