//简易的操作系统
/*----------------------------头文件----------------------------*/
#include <iostream> 
#include <vector> 
#include <functional>
#include <unistd.h>     // 提供：alarm、pause等系统调用
#include <signal.h>     // 提供：信号处理相关函数


/*----------------------------模拟进程控制块结构体 --> 用于描述进程的核心属性----------------------------*/
struct task_struct
{
    //1.进程ID
    //2.时间片计数器，本质是一个整数计数器
    //3.进程要执行的函数（函数指针）

    pid_t id;               
    int count = 5;         
    void(*code)();          
};
 
/*----------------------------模拟操作系统核心任务函数----------------------------*/
//1.模拟进程列表 ---> 存储系统中所有的进程任务
std::vector<task_struct> task_list;

//2.定义函数类型 ---> 用于存储系统任务的函数对象
using func_t = std::function<void()>;

//3.系统任务列表 ---> 存储所有需要周期性执行的操作系统任务
std::vector<func_t> funcs;

//4.时间戳 ---> 模拟系统运行的时间计数
int timestamp = 0;  

//(1)进程调度函数：模拟操作系统的进程调度逻辑
void Schedule()
{
    std::cout << "我是进程调度" << std::endl;
}

//(2)内存管理函数：模拟周期性内存检查任务
void MemManger()
{
    std::cout << "我是周期性的内存管理，正在检查有没有内存问题" << std::endl;
}

//(3)数据刷新函数：模拟定期将内存数据刷新到磁盘的任务
void Fflush()
{
    std::cout << "我是刷新程序，我在定期刷新内存数据，到磁盘" << std::endl;
}


/*----------------------------闹钟信号（SIGALRM）的处理函数----------------------------*/
void handlerSig(int sig)
{
    //1.时间戳递增记录系统运行的“时间单位”
    timestamp++;  

    //2.遍历并执行所有系统任务
    for(auto f : funcs)
    {
        f();
    }
    std::cout << "##############################" << std::endl;

    //3.重新设置1秒后触发闹钟信号（形成周期性执行）
    int n = alarm(1);
}


/*----------------------------主程序----------------------------*/
int main()
{
    //1.向系统任务列表中添加核心任务
    funcs.push_back(Schedule);
    funcs.push_back(MemManger);
    funcs.push_back(Fflush);

    //2.为闹钟信号（SIGALRM）注册处理函数
    signal(SIGALRM, handlerSig);

    //3.设置1秒后触发首次SIGALRM信号
    alarm(1); 

    //4.无限循环模拟操作系统的核心运行逻辑 ---> 等待信号触发任务
    while(true)
    {
        pause();  // 使进程暂停，直到收到任意信号才唤醒
    }

    return 0;
}