// 验证主线程等待的就是那个新线程

#include <iostream>    
#include <cstdio>      // 包含“C标准输入输出库” -----> 提供printf、snprintf等函数（格式化输出）
#include <unistd.h>    // 包含“Unix系统调用接口” ----> 提供sleep()函数（线程休眠）
#include <pthread.h>   // 包含“POSIX线程库头文件” ---> 提供线程创建、等待等函数（pthread_create/pthread_join等）


//1.打印线程ID（pthread_t类型），以十六进制格式输出
void showtid(pthread_t &tid)
{
    // 注意：%lx表示以十六进制长整数格式输出pthread_t类型（pthread_t通常为无符号长整数）
    printf("tid: 0x%lx\n", tid);
}

//2.将线程ID（pthread_t类型）格式化为十六进制字符串
std::string FormatId(const pthread_t &tid)
{
    //1.存储格式化后的字符串
    char id[64];  

    //2.用snprintf将tid按"0x%lx"格式写入id数组，确保不越界
    snprintf(id, sizeof(id), "0x%lx", tid);

    //3.返回转换为std::string类型的格式化后的字符串（如："0x7f8a1b2c3d4e"）
    return std::string(id);  
}

/*------------------------------------新线程执行函数------------------------------------*/
void *routine(void *args)
{
    //1.将void*类型参数转换为const char*，构造线程名称字符串
    std::string name = static_cast<const char *>(args);

    //2.获取当前线程的ID（pthread_self()返回调用线程的ID）
    pthread_t tid = pthread_self();

    //3.控制执行5次后退出
    int cnt = 5;  
    while (cnt)
    {
        std::cout << "我是一个新线程: 我的名字是: " << name << " 我的Id是: " << FormatId(tid) << std::endl;

        sleep(1);  // 休眠控制打印频率
        cnt--;     // 递减计数器
    }

    return (void*)123;  // 线程正常退出，返回退出码123（临时用void*存储整数）
    // 线程退出时的返回值（通过pthread_join可被主线程获取）
}

int main()
{
    //第一步：定义pthread_t类型变量tid ---> 存储新创建线程的ID
    pthread_t tid;  

    //第二步：创建新线程
    int n = pthread_create(&tid, nullptr, routine, (void *)"thread-1");

    showtid(tid); //打印新线程的ID（主线程视角获取的tid）

    //第三步：执行具体的事务
    int cnt = 5;  
    while (cnt)
    {
        std::cout << "我是main线程: 我的名字是: main thread" << " 我的Id是: " 
            << FormatId(pthread_self()) << std::endl;

        sleep(1);  
        cnt--;    
    }


    //第四步：等待新线程执行完毕（阻塞主线程，直到目标线程退出）
    pthread_join(tid, nullptr);
    /* pthread_join的参数介绍：
    *       1. 参数1：tid - 要等待的线程ID
    *       2. 参数2：&ret - 输出参数，接收线程的返回值（即routine函数的return值）
    *  注意事项：
    *       1. 若线程异常终止（如：触发信号），进程会直接退出，pthread_join无法执行
    *       2. 线程异常属于进程级事件，需通过信号处理机制处理，pthread_join不负责异常处理
    */

    return 0;
}
