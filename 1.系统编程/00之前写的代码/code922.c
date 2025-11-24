#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main()
{
    //1.打印提示信息，表明程序即将开始运行
    printf("我的程序要运行了!\n");

    //2.调用 fork() 创建子进程
    if (fork() == 0)
    {
        execl("/usr/bin/ls", "ls", "-l", "-a", NULL);

        exit(1); //注意：只有当 execl 执行失败时（返回 -1），才会执行 exit(1)，表示子进程异常退出
    }

    //3.父进程执行区域：等待任意一个子进程结束
    waitpid(-1, NULL, 0);
    /* 系统调用waitpid参数的介绍：
    *       参数 1: -1 —— 表示等待任意一个子进程
    *       参数 2: NULL —— 不关心子进程的退出状态
    *       参数 3: 0 —— 使用默认的阻塞等待方式
    */

    //4.子进程结束后，打印提示信息，表明程序运行完毕
    printf("我的程序运行完毕了\n");

    //5.程序正常结束，返回 0
    return 0;
}
