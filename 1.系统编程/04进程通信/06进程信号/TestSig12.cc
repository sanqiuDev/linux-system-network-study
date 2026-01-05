//怎么验证子进程退出的时候会给父进程发送SIGCHLD信号？

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

/*-------------------------------------信号处理函数-------------------------------------*/
void Say(int num)
{
    std::cout << "father get a signal: " << num << std::endl;
}

/*-------------------------------------主函数-------------------------------------*/
int main()
{
    //1.为SIGCHLD信号注册处理函数Say
    signal(SIGCHLD, Say); // SIGCHLD信号是子进程终止时向父进程发送的通知信号

    //2.创建子进程
    pid_t id = fork();
     
    //3.子进程执行分支
    if (id == 0)
    {
        //3.1：子进程输出自身状态
        std::cout << "I am child, exit" << std::endl;  

        //3.2：子进程休眠3秒模拟业务执行时间
        sleep(3);  

        //3.3：子进程以退出码3终止
        exit(3);   
    }

    //4.父进程执行：等待子进程终止（阻塞式等待）
    waitpid(id, nullptr, 0);

    //5.父进程输出自身终止信息
    std::cout << "I am fater, exit" << std::endl;  
    return 0;
}