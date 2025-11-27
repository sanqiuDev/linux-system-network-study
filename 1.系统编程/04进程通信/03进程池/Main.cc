#include "ProcessPool.hpp"  // 包含进程池类ProcessPool的头文件，提供进程池的相关声明

int main()
{
    //1.创建进程池对象pp
    ProcessPool pp(gdefaultnum);

    //2.启动进程池：初始化子进程，使其子进程进入等待状态，准备待接收并处理任务
    pp.Start();

    //3.自动向进程池派发任务
    int cnt = 5;  
    while(cnt--)   //循环5次，每次派发一个任务
    {
        pp.Run();  //调用进程池的Run方法，将一个任务添加到任务队列，由进程池中的子进程处理
        sleep(1);  //每次派发任务后休眠1秒，控制任务派发的频率
    }

    //4.回收资源并结束进程池：释放子进程相关资源
    pp.Stop();
    return 0;  
}

