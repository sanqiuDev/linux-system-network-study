//创建多线程时候的陷阱

#include <iostream>     // 包含“C++标准输入输出流库”
#include <vector>       // 包含“C++标准容器vector”
#include <unistd.h>     // 包含“Unix系统调用接口” ----> 提供sleep()函数（线程休眠）
#include <pthread.h>    // 包含“POSIX线程库头文件” ---> 提供线程创建、等待等函数
#include <cstdio>       // 包含“C标准输入输出库” -----> 提供snprintf()函数（格式化字符串）

//1.定义要创建的线程数量
const int num = 5;


/*------------------------------------新线程执行函数------------------------------------*/
void *routine(void *args)
{
    sleep(1);  // 线程休眠1秒，模拟初始化操作或等待其他线程

    //1.将void*类型参数转换为const char*，构造线程名称字符串
    std::string name = static_cast<const char *>(args);

    //2.执行5次后退出
    int cnt = 5;  
    while (cnt--)
    {
        std::cout << "new线程名字: " << name << std::endl;
        sleep(1);  
    }

    //3.线程正常退出，返回空指针
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

        //2.定义栈上的字符数组用于临时存储线程名称
        char id[64]; 
        /* 注意：当前代码使用的是栈上数组（局部变量），这存在潜在风险：
        *    1. 栈上变量的生命周期仅限于当前循环迭代，
        *    2. 当循环进入下一次迭代时，该数组的内存地址会被重复使用
        *    3. 由于线程创建后可能不会立即执行（取决于系统调度），
        *    4. 若线程启动时原栈上数组已被覆盖（后续迭代修改了该地址的数据），则线程会读取到错误的名称
        * 
        *    5. 正确的做法应使用堆分配（如：char* id = new char [64]），
        *    6. 确保每个线程的参数地址独立且生命周期足够长，避免因栈内存复用导致的参数错乱
        */

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