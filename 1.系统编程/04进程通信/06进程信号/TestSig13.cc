//怎么使用SIGCHLD信号的处理函数回收所有已终止的子进程？

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

/*-------------------------------------信号处理函数-------------------------------------*/
void WaitAll(int num)
{
    //1.循环回收子进程，直到没有可回收的子进程为止
    while (true) //细节一：这里要循环回收子进程 <--- 子进程可能有多个
    {
        //1.1：非阻塞式回收任意一个已终止的子进程（WNOHANG表示非阻塞）
        /*
        *    -1：表示回收任意子进程
        *    nullptr：表示不关心子进程退出状态
        *    WNOHANG：表示非阻塞
        */
        pid_t n = waitpid(-1, nullptr, WNOHANG);  //细节二：使用非阻塞等待 <--- 子进程不会一起退出
        
 
        //情况一：没有可回收的子进程，退出循环
        if (n == 0) 
        {
            break;
        }

        //情况二：waitpid调用失败，输出错误信息并退出循环
        else if (n < 0)
        {
            std::cout << "waitpid error " << std::endl;
            break;
        }

        //情况三：若n>0，说明成功回收了一个子进程，继续循环回收其他子进程
    }

    //2.输出父进程收到的信号编号，提示信号处理完成
    std::cout << "father get a signal: " << num << std::endl;
}

int main()
{
    //1.为SIGCHLD信号注册处理函数WaitAll
    signal(SIGCHLD, WaitAll); // SIGCHLD是子进程终止时向父进程发送的信号，注册后父进程可批量回收子进程

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