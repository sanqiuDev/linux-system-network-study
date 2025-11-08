//功能：实现刀枪不如的进程

#include <iostream>    
#include <unistd.h>    // 包含“Unix标准函数库” ---> 提供sleep()、getpid()等系统调用
#include <sys/types.h> // 包含“系统类型定义” ---> 如pid_t（进程ID类型）
#include <signal.h>    // 包含“信号处理相关函数库” ---> 提供signal()、raise()等信号操作函数

/*------------------------------------信号处理函数------------------------------------*/
void handlerSig(int sig) //参数sig：表示接收到的信号编号
{
    //1.打印收到的信号编号
    std::cout << "获得了一个信号: " << sig << std::endl;
}

/*------------------------------------主函数------------------------------------*/
int main()
{
    //1.循环注册1~31号信号处理函数
    for(int i = 1; i < 32; i++)
    {
        signal(i, handlerSig);  
    }

    //2.无限循环模拟进程持续运行
    int cnt = 0;
    while (true) 
    {
        //2.1：打印当前循环次数和进程ID（getpid()获取当前进程ID）
        std::cout << "hello world, " << cnt++ << " ,pid: " << getpid() << std::endl;

        //2.2：休眠1秒降低循环频率
        sleep(1);  
    }
}
