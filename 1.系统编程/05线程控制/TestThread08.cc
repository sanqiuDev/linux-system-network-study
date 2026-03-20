//pthread_detach系统调用的使用

#include <iostream>    // 包含“C++标准输入输出流库”
#include <unistd.h>    // 包含“Unix系统调用接口”
#include <pthread.h>   // 包含“POSIX线程库头文件” --> 提供线程创建、分离、等待等函数
#include <cstring>     // 包含“C字符串处理库” ------> 提供strerror()函数（将错误码转换为描述字符串）
 
/*------------------------------------新线程执行函数------------------------------------*/
void *routine(void *args)
{
    /*------------------分离新线程 ---> 第二版：新线程自我分离------------------*/
    pthread_detach(pthread_self()); //注意：分离后，线程结束时会自动释放资源，无需主线程调用pthread_join等待
    std::cout << "新线程被分离" << std::endl;

    int cnt = 5;  
    while (cnt--)
    {
        std::cout << "我是新线程" << ", pid: " << getpid() << std::endl;
        sleep(1);  
    }

    return nullptr;  
}


/*------------------------------------主程序------------------------------------*/
int main()
{
    /*------------------第一步：准备新线程------------------*/
    pthread_t tid;  

    /*------------------第二步：创建新线程------------------*/
    pthread_create(&tid, nullptr, routine, (void *)"thread-1");


    // /*------------------第三步：分离新线程 ---> 第一版：主线程主动分离新线程------------------*/
    // pthread_detach(tid);
    // std::cout << "新线程被分离" << std::endl;

    int cnt = 5;  
    while (cnt--)
    {
        std::cout << "我是主线程" << ", pid: " << getpid() << std::endl;
        sleep(1);  
    }

    
    /*------------------第四步：回收新线程 --->  尝试等待分离状态的线程------------------*/
    int n = pthread_join(tid, nullptr);
    if (n != 0)
    {
        // 等待失败：通过strerror(n)将错误码转换为可读字符串（如："Invalid argument"）
        std::cout << "pthread_join error: " << strerror(n) << std::endl;
    }
    else
    {
        // 等待成功：（本代码中不会执行到，因为线程已分离）
        std::cout << "pthread_join success: " << strerror(n) << std::endl;
    }

    return 0;
}