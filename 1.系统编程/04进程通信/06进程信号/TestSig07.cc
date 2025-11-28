//管理进程信号屏蔽集和未决信号集的系统调用的使用

#include <iostream>    
#include <unistd.h>     
#include <signal.h>     
#include <sys/types.h>  

/*-------------------------------------打印进程的未决信号集-------------------------------------*/
void PrintPending(sigset_t &pending)
{
    //1.打印未决信号集对应的进程的pid
    std::cout << "当前进程(" << getpid() << ") pending信号集: ";

    //2.从31号信号到1号信号依次检查（信号编号范围1-31）
    for (int signo = 31; signo >= 1; signo--)
    {
        //3.检查信号signo是否在pending集合中
        //情况一：在pengding信号集中
        if (sigismember(&pending, signo))
        {
            std::cout << "1";  // 1表示信号已产生（未决）
        }

        //情况二：不在pengding信号集中
        else
        {
            std::cout << "0";  // 0表示信号未产生
        }
    }
    std::cout << std::endl;
}

/*-------------------------------------信号处理函数-------------------------------------*/
void handler(int signo)
{
    //1.打印被递达的信号的的编号
    std::cout << signo << " 号信号被递达!!!" << std::endl;
    std::cout << "---------------------------" << std::endl;
    
    //2.获取当前进程的pending信号集并进行打印 ---> 为了验证：pending表的修改是在信号递达前or后？（答案：之前）
    sigset_t pending;
    sigpending(&pending);
    PrintPending(pending);  

    std::cout << "---------------------------" << std::endl;
}


/*-------------------------------------主程序-------------------------------------*/
int main()
{
    /*---------------------第一步：捕捉2号信号---------------------*/
    signal(2, handler);  


    /*---------------------第二步：屏蔽2号信号---------------------*/
    //1.创建并初始化信号屏蔽集
    sigset_t block_set, old_set;
    sigemptyset(&block_set);   // 清空block_set集合
    sigemptyset(&old_set);     // 清空old_set集合

    //2.向block_set中添加2号信号（准备屏蔽2号信号）
    sigaddset(&block_set, SIGINT);  

    //3.真正修改当前进程的内核block表，完成对2号信号的屏蔽
    sigprocmask(SIG_SETMASK, &block_set, &old_set);  


    /*---------------------第三步：屏蔽2号信号---------------------*/
    int cnt = 15;
    while (true)
    {
        //1.获取当前进程的pending信号集
        sigset_t pending;
        sigpending(&pending);  

        //2.打印pending信号集（观察信号是否被屏蔽）
        PrintPending(pending);  
        /* 可以观察到：
        *     1. 信号未产生时该进程的pending表
        *     2. 信号产生后该进程的pending表
        *     3. 信号解除阻塞时该进程的pending表
        */

        //3.计数器减减 
        cnt--;

        //4.当计数器为0时，解除对2号信号的屏蔽
        if (cnt == 0)
        {
            //4.1：打印2号信号解除屏蔽
            std::cout << "解除对2号信号的屏蔽!!!" << std::endl;
            
            //4.2：恢复信号屏蔽集为旧集合old_set（即取消对2号信号的屏蔽）---> pending表会发生变化
            sigprocmask(SIG_SETMASK, &old_set, &block_set);  
        }
        sleep(1);  // 每秒循环一次
    }
    return 0;
}