//使用系统调用clone创建线程

#include <stdio.h>    // 提供printf函数
#include <stdlib.h>   // 提供malloc、free、exit等内存管理和进程退出函数
#include <unistd.h>   // 提供sleep、getpid等系统调用
#include <sched.h>    // 提供clone系统调用的声明
#include <sys/wait.h> // 提供waitpid函数，用于等待子进程退出

//1.定义子进程栈空间大小为1MB
#define STACK_SIZE (1024 * 1024) 
 
/*------------------------------------子进程执行函数------------------------------------*/
static int child_func(void *arg)
{
    while (true) 
    {
        printf("子进程的PID = %d\n", getpid());
        sleep(1); 
    }
    return 0;
}

/*------------------------------------主函数------------------------------------*/
int main()
{
    //1.为子进程分配1MB的栈空间
    char *stack = (char *)malloc(STACK_SIZE);
    if (stack == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    //2.使用clone系统调用创建子进程
    pid_t pid = clone(child_func, stack + STACK_SIZE, CLONE_VM | SIGCHLD, NULL);
    if (pid == -1) //注意：处理子进程创建失败的情况
    {
        //1.打印错误的消息
        perror("clone");

        //2.释放已分配的栈空间
        free(stack); 

        //3.以失败状态退出进程
        exit(EXIT_FAILURE); //注意：EXIT_FAILURE是标准错误退出码（通常定义为1）
    } 
    /* 系统调用clone的参数介绍：
    *     1. 参数1：子进程执行的函数
    *     2. 参数2：子进程栈的起始地址（栈是向下增长的，所以要指向栈顶，栈顶时高地址，而创建出的栈的指针指向的是低地址的栈底，所以要stack + STACK_SIZE）
    *     3. 参数3：clone标志，CLONE_VM表示父子进程共享虚拟内存，SIGCHLD表示子进程结束时向父进程发送SIGCHLD信号
    *     4. 参数4：传递给子进程函数的参数（此处为NULL）
    */


    //3.打印父进程和子进程的PID
    printf("父进程的PID = %d, 子进程的PID = %d\n", getpid(), pid);

    //4.等待子进程结束
    if (waitpid(pid, NULL, 0) == -1) //注意：处理子进程等待失败的情况 
    {
        //1.打印错误的消息
        perror("waitpid");

        //2.释放已分配的栈空间
        free(stack); 

        //3.以失败状态退出进程
        exit(EXIT_FAILURE);
    }

    //5.释放为子进程分配的栈空间
    free(stack); 
    return 0;
}