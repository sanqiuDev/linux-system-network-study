//怎么做才能使得fork出来的子进程能在终止的时候自动进行清理呢？

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


int main()
{
    //1.为SIGCHLD信号注册处理函数WaitAll
    signal(SIGCHLD, SIG_IGN); 

    //2.循环创建5个子进程
    for (int i = 0; i < 5; i++)
    {
        //创
        pid_t id = fork(); 
        
        //子
        if (id == 0)
        { 
            sleep(3); 
            std::cout << "I am child, exit" << std::endl; 
            exit(3); 
        }
    }

    //父
    while (true)
    {
        std::cout << "I am fater, exit" << std::endl; 
        sleep(1); 
    }

    return 0;
}