//使用pthread_cancel函数

#include <iostream>   
#include <unistd.h>   // 包含“Unix系统调用接口” 
#include <pthread.h>  // 包含“POSIX线程库头文件” 

 
/*------------------------------------新线程执行函数------------------------------------*/
void *routine(void *args) // args：传递给新线程的参数（由pthread_create的第四个参数传入）
{
    //1.将传入的void*类型参数转换为const char*，并构造std::string类型的线程名称
    std::string name = (const char*)args;
    
    //2.新线程的无限循环，保持线程持续运行
    while(true)
    {
        //2.1：线程休眠1秒，避免高频打印占用CPU资源
        sleep(1);   
        
        //2.2：打印新线程信息：线程名称 + 所属进程ID（getpid()返回当前进程的PID）
        std::cout << "我是新线程: name: " << name << ", pid: " << getpid() << std::endl;
           
    }
    
    //3.线程函数返回值（pthread_join可获取）
    return nullptr;  
}


int main()
{
    /*------------------第一步：准备新线程------------------*/
    pthread_t tid;

    /*------------------第二步：创建新线程------------------*/
    pthread_create(&tid, nullptr, routine, (void *)"thread-1");

    sleep(3); // 主线程休眠3秒，确保新线程有足够时间执行

    /*------------------第三步：取消新线程------------------*/
    pthread_cancel(tid);
    std::cout << "新线程被取消" << std::endl; // 打印新线程被取消的提示信息

    /*------------------第三步：回收新线程------------------*/
    //1.定义void*指针，用于接收线程的退出返回值
    void *ret = nullptr;

    //2.等待新线程执行完毕（阻塞主线程）
    pthread_join(tid, &ret);

    //3.打印新线程的执行结果（将void*转换为long long类型显示）
    std::cout << "新线程结束，运行结果: " << (long long)ret << std::endl;

    return 0;
}