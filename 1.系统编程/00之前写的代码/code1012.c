#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

//1.定义全局变量，初始值为100
int gval = 100;

int main()
{
    //2.调用fork()函数创建子进程，fork()会返回两次
    pid_t id = fork();

    //3.子进程执行的代码分支（id为0表示当前是子进程）
    if (id == 0)
    {
        // 子进程进入死循环
        while (1)
        {
            //3.1：打印子进程中全局变量gval的值、gval的地址、子进程的PID、子进程的父进程PID
            printf("子: gval: %d, &gval: %p, pid: %d, ppid: %d\n",gval, &gval, getpid(), getppid());
            
            //3.2：休眠1秒，控制输出频率
            sleep(1); 
            
            //3.3：子进程中对全局变量gval进行自增操作
            gval++;    
        }
    }
    //4.父进程执行的代码分支（id不为0表示当前是父进程）
    else
    {
        // 父进程进入死循环
        while (1)
        {
            //4.1：打印父进程中全局变量gval的值、gval的地址、父进程的PID、父进程的父进程PID
            printf("父: gval: %d, &gval: %p, pid: %d, ppid: %d\n",gval, &gval, getpid(), getppid());
            
            //4.2：休眠1秒，控制输出频率
            sleep(1);  
        }
    }

    return 0;
}


