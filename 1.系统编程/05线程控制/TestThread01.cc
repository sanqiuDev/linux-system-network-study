//使用pthread_create创建线程

#include <iostream>   
#include <unistd.h>   // 包含“Unix系统调用接口” ----> 用于sleep()函数（延时）、getpid()函数（获取进程ID）
#include <pthread.h>  // 包含“POSIX线程库头文件” ---> 用于线程创建（pthread_create）等线程操作
 

/*------------------------------------新线程执行函数------------------------------------*/
void *threadrun(void *args) // args：传递给新线程的参数（由pthread_create的第四个参数传入）
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
        
        // //2.3：定义局部变量a并初始化为10
        // int a = 10;    
        
        // //2.4：故意制造除零错误 ---> 触发算术异常，导致线程崩溃（进而可能终止整个进程）
        // a /= 0;        
    }
    
    //3.线程函数返回值（pthread_join可获取）
    return nullptr;  
}

/*------------------------------------主函数------------------------------------*/
int main()
{
    //第一步：定义pthread_t类型变量tid ---> 用于存储新创建线程的线程ID（POSIX线程标识）
    pthread_t tid;  
    
    //第二步：创建新线程：
    pthread_create(&tid, nullptr, threadrun, (void*)"thread-1");
    /* pthread_create的参数说明：
    *      1. 参数1：tid的地址，用于输出新线程的ID
    *      2. 参数2：线程属性（nullptr 表示使用默认属性，如：栈大小、调度策略等）
    *      3. 参数3：新线程要执行的函数指针（threadrun）
    *      4. 参数4：传递给threadrun函数的参数（此处传入字符串"thread-1"作为线程名称）
    */

    //第三步：主线程的无限循环，保持主线程持续运行
    while(true)
    {
        //1.打印主线程信息：标识 + 所属进程ID（与新线程PID相同，因同一进程内所有线程共享PID）
        std::cout << "我是主线程..." << ", pid: " << getpid() << std::endl;

        //2.主线程休眠1秒，与新线程打印频率保持一致
        sleep(1);  
    }
    
    return 0;  
}

