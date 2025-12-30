//怎么使用sigaction这个系统调用？

#include <iostream>     
#include <unistd.h>     // 提供POSIX操作系统API（如sleep、getpid）
#include <signal.h>     // 提供“信号处理相关函数” ---> 如sigaction、sigpending等）
#include <sys/types.h>  // 提供“基本数据类型定义” ---> 如pid_t

/*------------------------------------信号处理函数------------------------------------*/
void handler(int signum)
{
    //1.输出当前触发的信号编号，提示信号处理开始
    std::cout << "hello signal: " << signum << std::endl;

    //2.死循环持续监测进程的未决信号集（pending表）
    while(true)
    {
        //2.1：定义一个sigset_t类型的变量pending，用于存储当前进程的未决信号集
        sigset_t pending;

        //2.2：调用sigpending函数：获取当前进程的未决信号集，存入pending变量
        sigpending(&pending);

        //2.3：遍历信号编号1~31（常见信号集中的有效信号范围）
        for(int i = 31; i >= 1; i--)
        { 
            // 调用sigismember函数：检查信号i是否在pending集合中
            if(sigismember(&pending, i))
                std::cout << "1";  // 信号i“处于”未决状态，输出1
            else
                std::cout << "0";  // 信号i“未处于”未决状态，输出0
        }

        //2.4：每行输出结束后换行，方便观察
        std::cout << std::endl;
        //2.5：休眠1秒，避免输出过快
        sleep(1);
    }
}

/*------------------------------------主函数------------------------------------*/
int main()
{
    /*-------------------------第一步：创建sigaction结构体-------------------------*/
    //1.定义两个sigaction结构体 ---> act用于设置新的信号处理方式，oact用于保存旧的设置
    struct sigaction act, oact;

    /*-------------------------第二步：设置信号处理函数-------------------------*/
    //2.设置信号处理函数 ---> 当收到指定信号时，调用handler函数
    act.sa_handler = handler;

    /*-------------------------第三步：初始化信号屏蔽集和信号处理的标志位-------------------------*/
    //3.初始化信号屏蔽集（sa_mask）---> 清空集合，确保初始状态无屏蔽信号
    sigemptyset(&act.sa_mask);

    //4.向信号屏蔽集中添加信号3和4 ---> 表示执行handler时，会额外屏蔽3号和4号信号
    sigaddset(&act.sa_mask, 3);
    sigaddset(&act.sa_mask, 4);

    //5.设置信号处理的标志位为0表示使用默认行为
    act.sa_flags = 0; //如信号处理期间自动屏蔽当前信号，处理完成后解除屏蔽

    /*-------------------------第四步：调用sigaction函数-------------------------*/
    //6.调用sigaction函数
    //特别注意：执行handler期间，会自动屏蔽当前信号（SIGINT，2号）+ sa_mask中的信号（3、4号）
    sigaction(SIGINT, &act, &oact);

    //7.主循环：持续输出进程ID，证明程序在正常运行
    while(true)
    {
        std::cout << "hello world: " << getpid() << std::endl;
        sleep(1);
    }
    return 0;
}