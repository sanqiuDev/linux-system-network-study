//功能：排查出62个信号中哪些信号是不能进行捕捉的

#include <iostream>    
#include <unistd.h>    // 包含“Unix标准函数库” ---> 提供sleep()、getpid()等系统调用
#include <sys/types.h> // 包含“系统类型定义” ---> 如pid_t（进程ID类型）
#include <signal.h>    // 包含“信号处理相关函数库” ---> 提供signal()、raise()等信号操作函数

/*------------------------------------信号处理函数------------------------------------*/
void handlerSig(int sig) //参数sig：表示接收到的信号编号
{
    std::cout << "获得了一个信号: " << sig << std::endl;
}

/*------------------------------------主函数------------------------------------*/
int main()
{
    //1.循环注册信号处理函数：为1~31号信号（常见信号范围）绑定自定义处理函数handlerSig
    for(int i = 1; i < 32; i++)
        signal(i, handlerSig); 


    //2.测试主动发送信号
    for(int i = 1; i < 32; i++)
    {
        //2.1：每秒发送一个信号
        sleep(1);

        /*不断地修改下面的内容去寻找无法被捕获的信号有哪些？
        //2.2：跳过9号（SIGKILL）和19号（SIGSTOP）信号（无法被捕获）
        if(i == 9 || i == 19)  
        {
            continue;
        }
        */

        //2.3：向当前进程发送信号i（raise()函数用于进程向自身发送信号）
        raise(i); 
    }


    //3.无限循环模拟进程持续运行
    int cnt = 0;  
    while (true) 
    {
        //3.1：打印当前循环次数和进程ID（getpid()获取当前进程ID）
        std::cout << "hello world, " << cnt++ << " ,pid: " << getpid() << std::endl;

        //3.2：休眠1秒降低循环频率
        sleep(1);  
    }
}