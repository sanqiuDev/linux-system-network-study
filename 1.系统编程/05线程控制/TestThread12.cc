//每一个线程可以访问另一个线程的所有的资源包括栈

#include <iostream>   
#include <unistd.h>   // 包含“Unix系统调用接口” 
#include <pthread.h>  // 包含“POSIX线程库头文件” 


//1.定义全局指针变量 ---> 用于在主线程和子线程间共享地址
int *p = nullptr;

/*------------------------------------新线程执行函数------------------------------------*/
void *threadrun(void *args)
{
    //1.子线程中的局部变量a（存储在子线程的私有栈中）
    int a = 123;

    //2.将全局指针p指向子线程局部变量a的地址
    p = &a;

    //3.子线程进入无限循环，每休眠1秒 ---> 保持线程不退出（避免局部变量a被释放）
    while(true) {sleep(1);}
}

/*------------------------------------主函数------------------------------------*/
int main()
{
    //1.定义线程ID变量（pthread_t类型，用于标识子线程）
    pthread_t tid;

    //2.创建子线程
    pthread_create(&tid, nullptr, threadrun, nullptr);

    //3.主线程进入无限循环，每秒打印一次*p的值
    while(true)
    {
        //3.1：访问全局指针p指向的地址（子线程中的局部变量a）并打印其值
        std::cout << "*p : " << *p << std::endl;

        //3.2：休眠1秒，控制打印频率
        sleep(1);  
    }

    //4.等待子线程退出
    pthread_join(tid, nullptr);
    
    return 0;
}