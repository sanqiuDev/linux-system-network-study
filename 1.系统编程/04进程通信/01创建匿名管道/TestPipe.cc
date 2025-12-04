#include <iostream> 
#include <unistd.h> // 包含 pipe 系统调用的头文件

int main()
{
    //1.定义数组fds用于存储管道的读端和写端文件描述符 
    int fds[2] = {0};
    //2.调用pipe系统调用创建匿名管道，成功返回 0，失败返回 -1
    int n = pipe(fds);

    //3.判断管道创建是否失败
    if(n < 0)
    {
        //3.1：输出错误信息到标准错误流
        std::cerr << "pipe error" << std::endl; 

        //3.2：函数返回 1，表示表示程序执行异常结束
        return 1;
    }

    //4.输出管道读端的文件描述符
    //5.输出管道写端的文件描述符
    std::cout << "fds[0]: " << fds[0] << std::endl;
    std::cout << "fds[1]: " << fds[1] << std::endl;

    return 0;
}