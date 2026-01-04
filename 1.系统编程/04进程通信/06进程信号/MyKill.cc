// 程序功能：模拟系统kill命令，向指定进程发送指定信号

#include <iostream>    
#include <string>        
#include <sys/types.h>    // 包含“系统类型定义” ---------> 提供pid_t（进程ID类型）的定义
#include <signal.h>       // 包含“信号处理相关函数库” ---> 提供kill()系统调用的声明

// 用法：./mykill 信号编号 目标进程PID
int main(int argc, char *argv[])
{
    //1.检查命令行参数数量是否正确
    if(argc != 3)
    { 
        std::cout << "./mykill sign pid" << std::endl; 
        return 1;  
    }

    //2.将命令行参数中的信号编号（字符串类型）转换为整数
    int signum = std::stoi(argv[1]);
    
    //3.将命令行参数中的目标进程PID（字符串类型）转换为pid_t类型（进程ID类型）
    pid_t target = std::stoi(argv[2]);

    //4.调用kill系统调用：向目标进程（target）发送指定信号（signum）
    int n = kill(target, signum); //注意：kill系统调用的参数传递顺序和kill命名的参数顺序不同
    if(n == 0)
    {
        std::cout << "send " << signum << " to " << target << " success.";
    }

    return 0;  
}
