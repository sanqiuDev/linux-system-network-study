#include <unistd.h>     // 包含 sleep 等函数声明
#include <sys/types.h>  // 包含 pid_t 类型定义
#include <sys/wait.h>   // 包含 waitpid 等进程等待相关函数声明
#include <stdio.h>      // 包含 printf 等标准输入输出函数声明
#include <errno.h>      // 包含 errno 错误码定义
#include <string.h>     // 包含 strerror 函数声明（用于将错误码转为错误信息字符串）
#include <stdlib.h>  // 添加这行，包含 exit 函数的声明

int main()
{
    //1.调用 fork 创建子进程
    pid_t id = fork();

    //2.子进程执行的代码段
    if (id == 0)
    {

        //2.1：定义计数器 cnt，用于控制子进程循环次数
        int cnt = 3;
        while (cnt)
        {
            //第一步：打印子进程的 PID（进程 ID）和 PPID（父进程 ID）
            printf("我是一个子进程，pid：%d, ppid：%d\n", getpid(), getppid());

            //第二步：子进程休眠 1 秒，控制打印频率
            sleep(1);

            //第三步：计数器减 1
            cnt--;
        }
        //2.2：子进程调用 exit 函数终止，参数 1 作为退出码
        exit(1);
    }

    //3.父进程执行的代码段
    //3.1：定义 status 变量，用于存储子进程的退出状态信息
    int status = 0;

    //3.2：调用 waitpid 等待指定子进程（pid 为 id）退出，0 表示阻塞等待
    pid_t rid = waitpid(id, &status, 0);

    // 情况一：waitpid 成功，返回值 rid 为已终止子进程的 PID
    if (rid > 0)
    {
        // status & 0x7F 的解析逻辑：
        // 1. 0x7F 是十六进制常量，对应二进制 00000000 00000000 00000000 01111111
        // 2. 按位与（&）操作会保留 status 变量的低 7 位，屏蔽高 25 位
        // 3. 这低 7 位正是 status 位图中存储“终止信号编号”的区域
        //    - 若结果为 0：表示子进程正常终止，未收到任何终止信号
        //    - 若结果为非 0：表示子进程因该信号编号对应的信号而异常终止
        //    （例如结果为 2 对应 SIGINT 信号，11 对应 SIGSEGV 信号）
        printf("wait success, rid: %d, exit code: %d, exit signal: %d\n", rid, (status >> 8) & 0xFF, status & 0x7F); 
        /*
         * 补充说明：
         *  ① 此解析方式仅反映 status 低 7 位的原始值，实际使用时需结合场景判断：
         *      1. 应先用 WIFSIGNALED(status) 宏判断子进程是否因信号终止
         *      2. 若为真，再用 WTERMSIG(status) 宏获取信号编号（内部封装了 status & 0x7F 的逻辑）
         *              例如：WTERMSIG(status) 等价于 status & 0x7F
         *  ② 0x7F 的取值源于信号编号的范围（Linux 系统中信号编号最大为 64，用 7 位足以表示）
         */
    }

    //情况二：waitpid 失败，打印错误码和对应的错误信息
    else
    {
        printf("wait failed: %d: %s\n", errno, strerror(errno));
    }

    //4.主函数返回 0，进程正常退出
    return 0;
}
 
