//IO效率
#include <iostream>    
#include <unistd.h>    
#include <signal.h>    

//1.定义全局的计数器用于记录循环次数
int cnt = 0;

//2.信号处理函数
void handlerSig(int sig)
{
    std::cout << "获得了一个信号: " << sig << " cnt: " << cnt << std::endl; //注意：只打印一次
    exit(13);
}

int main()
{
    //1.为SIGALRM信号（闹钟信号）注册handlerSig处理函数
    signal(SIGALRM, handlerSig);

    //2.设定1秒的闹钟，1秒后当前进程会收到SIGALRM信号
    alarm(1); 
 
    //3.仅递增计数器无IO操作
    while (true)
    {
        cnt++;
    }
}






















// #include <iostream>    
// #include <unistd.h>    
// #include <signal.h>    

// int main()
// {
//     //1.设定1秒的闹钟，1秒后当前进程会收到SIGALRM信号
//     alarm(1);

//     //2.循环向显示器上打印输出的次数
//     int cnt = 0;
//     while (true)
//     {
//         std::cout << "count: " << cnt++ << std::endl;
//     }
// }