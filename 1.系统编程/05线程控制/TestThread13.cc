//线程局部存储如何使用？

#include <iostream>   
#include <string>   
#include <unistd.h>    // 提供“sleep函数” ---> 用于线程休眠 
#include <pthread.h>   // 提供“POSIX线程库核心接口” ---> 线程创建、等待等


//1.定义线程局部存储变量 ---> 用__thread关键字修饰
__thread int count = 1; // 核心特性：每个线程拥有该变量的独立副本，互不干扰（看似全局，实则线程私有）

 
//2.“辅助函数” ---> 将变量的内存地址转换为字符串形式（便于打印查看）
std::string Addr(int &c)
{
    //1.存储地址字符串的缓冲区
    char addr[64];  

    //2.格式化输出：将变量c的地址（%p格式）写入addr缓冲区
    snprintf(addr, sizeof(addr), "%p", &c);

    //3.转换为std::string返回
    return addr;    
}

/*------------------------------------新线程执行函数------------------------------------*/
//1.线程1的入口函数 ---> 修改并打印自己的count副本
void *routine1(void *args)
{
    (void)args;  // 忽略传入的参数（此处未使用，避免编译警告）
    
    while (true)  
    {
        //1.打印线程1的count值、修改提示及count的地址
        std::cout << "thread - 1, count = " << count << "[我来修改count], "
                  << "&count: " << Addr(count) << std::endl;
        //2.仅修改线程1自己的count副本（不影响线程2）          
        count++;  

        //3.休眠1秒，控制打印频率
        sleep(1); 
    }
}

//2.线程2的入口函数 ---> 仅打印自己的count副本（不修改）
void *routine2(void *args)
{
    (void)args;  // 忽略传入的参数（此处未使用，避免编译警告）
    
    while (true)  
    {
        //1.打印线程2的count值及count的地址
        std::cout << "thread - 2, count = " << count
                  << ", &count: " << Addr(count) << std::endl;

        //2.休眠1秒，控制打印频率
        sleep(1); 
    }
}

/*------------------------------------主函数------------------------------------*/
int main()
{
    //1.定义两个线程ID变量（用于标识线程）
    pthread_t tid1, tid2;  
    
    //2.创建两个新线程
    pthread_create(&tid1, nullptr, routine1, nullptr); // 创建线程1：绑定入口函数routine1，无参数传入
    pthread_create(&tid2, nullptr, routine2, nullptr); // 创建线程2：绑定入口函数routine2，无参数传入

    //3.阻塞等待两个新线程
    pthread_join(tid1, nullptr); // 阻塞等待线程1退出
    pthread_join(tid2, nullptr); // 阻塞等待线程2退出

    //4.主线程退出
    return 0;  
}

