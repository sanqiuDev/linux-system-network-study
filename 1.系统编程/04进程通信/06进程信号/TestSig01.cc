//功能：验证ctrl+c是一个信号

#include <iostream>    
#include <unistd.h>    // 包含“Unix系统函数头文件” --> 提供getpid()、sleep()等系统调用
#include <signal.h>    // 包含“信号处理头文件” ------> 提供signal()、信号常量（如SIGINT）等

/*------------------------------------信号处理函数------------------------------------*/
void handler(int signum) //参数signum：表示接收到的信号编号
{
    std::cout << "我是进程：" << getpid() << "，我获得了一个信号：" << signum << std::endl;
}

/*------------------------------------主函数------------------------------------*/
int main()
{
    //1.打印当前进程的ID，方便识别和外部操作（如kill命令）
    std::cout << "我是进程：" << getpid() << std::endl;

    //2.为SIGINT信号注册自定义处理函数handler
    signal(SIGINT, handler);

    //3.无限循环，模拟进程持续运行并等待信号
    while(true)
    {
        std::cout << "我是一个进程, 我正在等待一个信号" << std::endl;
        sleep(1); 
    }
}