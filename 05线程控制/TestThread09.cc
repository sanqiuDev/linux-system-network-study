//创建多线程

#include <iostream>     // 包含“C++标准输入输出流库”
#include <vector>       // 包含“C++标准容器vector”
#include <unistd.h>     // 包含“Unix系统调用接口” ----> 提供sleep()函数（线程休眠）
#include <pthread.h>    // 包含“POSIX线程库头文件” ---> 提供线程创建、等待等函数
#include <cstdio>       // 包含“C标准输入输出库” -----> 提供snprintf()函数（格式化字符串）

//1.定义要创建的线程数量
const int num = 10;


/*------------------------------------新线程执行函数------------------------------------*/
void *routine(void *args)
{
    sleep(1);  // 线程休眠1秒，模拟初始化操作或等待其他线程

    //1.将void*类型参数转换为const char*，构造线程名称字符串
    std::string name = static_cast<const char *>(args);

    //2.释放堆上分配的线程名称字符串（在主线程中通过new创建），避免内存泄漏
    delete (char*)args;

    //3.执行5次后退出
    int cnt = 5;  
    while (cnt--)
    {
        std::cout << "new线程名字: " << name << std::endl;
        sleep(1);  
    }

    //4.线程正常退出，返回空指针
    return nullptr;  
}


/*------------------------------------主函数------------------------------------*/
int main()
{
    /*---------------------第一步：定义vector容器---------------------*/
    std::vector<pthread_t> tids;

    /*---------------------第二步：循环创建num个线程---------------------*/
    for (int i = 0; i < num; i++)
    {
        //1.定义pthread_t类型变量tid ---> 存储新创建线程的ID
        pthread_t tid;  

        //2.在堆上分配字符数组 ---> 存储线程名称（如："thread-0"、"thread-1"等）
        char *id = new char[64]; //注意：必须用堆分配：若用栈变量，循环结束后地址会被复用，导致线程参数错乱
        //3.格式化线程名称，写入id数组（格式为"thread-序号"）
        snprintf(id, 64, "thread-%d", i); 

        //4.创建新线程
        int n = pthread_create(&tid, nullptr, routine, id);
        if (n == 0) 
        {
            tids.push_back(tid); //若线程创建成功，将其ID存入tids容器
        }
        else 
        {
            continue;  // 创建失败则跳过，继续创建下一个线程
        }

    }

    /*---------------------第三步：循环等待所有线程执行完毕---------------------*/
    for (int i = 0; i < tids.size(); i++)
    {
        //1.等待指定线程结束（阻塞主线程，直到tids[i]对应的线程退出）
        int n = pthread_join(tids[i], nullptr);
        if (n == 0)
        {
            std::cout << "等待新线程成功" << std::endl;
        }
    }

    return 0;
}