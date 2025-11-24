#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

//定义全局变量
int gval = 100;

int main()
{
    //1.打印父进程开始运行的信息
    printf("父进程开始运行,pid: %d\n", getpid());

    //2.调用fork()创建子进程
    pid_t id = fork();

    //3.fork()调用失败的情况处理
    if (id < 0)
    {
        // perror输出错误信息
        perror("fork");
        return 1;
    }

    //4.子进程执行的分支（fork()返回0）
    else if (id == 0)
    {
        //4.1：打印子进程的PID、父进程PID、全局变量gval的初始值
        printf("我是一个子进程！, 我的pid: %d, 我的父进程id: %d, gval: %d\n", getpid(), getppid(), gval);
        
        //4.2：子进程睡眠5秒，可用于观察父子进程执行顺序等情况
        sleep(5);

        //4.3：子进程进入死循环
        while (1)
        {
            //4.3.1：每次循环睡眠1秒
            sleep(1);

            //4.3.2：打印子进程修改变量前和修改后的值（这里是模拟展示，实际gval是子进程自己的副本）
            printf("子进程修改变量：%d->%d", gval, gval + 10);

            //4.3.3：子进程修改自己的gval副本
            gval += 10;

            //4.3.4：打印子进程的PID、父进程PID、子进程中的gval的值
            printf("我是一个子进程！, 我的pid: %d, 我的父进程id: %d, gval: %d\n", getpid(), getppid(),gval);
        }
    }

    //5.父进程执行的分支（fork()返回子进程ID，大于0）
    else
    {
        //5.1：父进程进入死循环
        while (1)
        {
            //5.1.1：每次循环睡眠1秒
            sleep(1);

            //5.1.2：打印父进程的PID、父进程的父进程PID、父进程中的gval的值
            printf("我是一个父进程！, 我的pid: %d, 我的父进程id: %d, gval: %d\n", getpid(), getppid(), gval);
        }
    }
    return 0;
}
