#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h> 

int main()
{
    //1.调用fork()创建子进程
    pid_t id = fork();

    //2.子进程执行的代码段
    if (id == 0)
    {
        //2.1：定义计数器cnt，用于控制子进程循环次数
        int cnt = 5;
        while (cnt)
        {
            //第一步：打印子进程的 PID（进程 ID）和 PPID（父进程 ID）
            printf("我是一个子进程，pid：%d, ppid：%d\n", getpid(), getppid());

            //第二步：子进程休眠 1 秒，控制打印频率
            sleep(1);

            //第三步：计数器减 1
            cnt--;
        }
        //2.2：子进程正常退出，参数 0 表示退出状态为成功
        exit(0);
    }

    //3.父进程执行的代码段，先休眠 10 秒，确保子进程先运行一段时间
    sleep(10);

    //4.调用 wait() 等待子进程退出，NULL 表示不关心子进程的退出状态
    pid_t rid = wait(NULL);
    if (rid > 0)
    {
        // wait() 成功返回子进程的 PID，打印提示信息
        printf("wait success, rid: %d\n", rid);
    }

    //5.父进程在 wait() 之后再休眠 10 秒
    sleep(10);
    //6.主函数返回 0，进程正常退出
    return 0;
}
