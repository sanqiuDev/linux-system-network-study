//使用C++11标准线程库创建线程
#include <iostream>  
#include <unistd.h>  // 包含“Unix系统调用接口” ---> 提供sleep()（线程休眠）和getpid()（获取进程ID）函数
#include <thread>    // 包含“C++11标准线程库” ----> 提供std::thread类用于创建和管理线程


/*------------------------------------新线程执行函数------------------------------------*/
void hello()
{
    while (true)
    {
        std::cout << "新线程: hello world, pid: " << getpid() << std::endl;
        sleep(1);  
    }
}

/*------------------------------------主函数------------------------------------*/
int main()
{
    //1.创建新线程
    std::thread t(hello); // std::thread t(hello) 表示构造一个线程对象t，新线程启动后会执行hello()函数

    //2.主线程的无限循环
    while (true)
    {
        std::cout << "我是主线程..." << ", pid: " << getpid() << std::endl;
        sleep(1);  
    }

    //3.等待新线程执行完毕（调用join()会阻塞主线程，直到t对应的线程结束）
    t.join();
    
    return 0;  
}