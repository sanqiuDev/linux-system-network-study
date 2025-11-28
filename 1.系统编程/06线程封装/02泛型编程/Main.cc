#include <unistd.h>     // 提供“Unix系统调用” ---> 如：sleep函数，用于线程休眠
#include "Thread.hpp"   // 包含自定义的Thread类模板头文件

using namespace ThreadModlue;  


//1.自定义线程数据类：用于向线程函数传递多个参数
class ThreadData
{
public:
    //1.线程ID
    //2.线程名称
    
    pthread_t tid;       
    std::string name;    
};

//2.线程执行的函数：接收ThreadData类型的参数
void Count(ThreadData td)
{
    while (true)  
    {
        std::cout << "我是一个新线程" << std::endl;
        sleep(1); 
    }
}


int main()
{
    /*---------------------【使用“自定义对象”作为参数创建线程】---------------------*/
    //1.创建线程数据对象
    ThreadData td;  

    //2.实例化Thread对象 ---> 模板参数为ThreadData，绑定Count函数和td对象
    Thread<ThreadData> t(Count, td);

    //3.创建线程
    t.Create();     

    sleep(3);  // 主线程休眠3秒，让子线程运行一段时间

    //4.强制终止线程（调用pthread_cancel）
    t.Cancel();     

    sleep(3);   // 主线程再休眠3秒   

    //5.等待线程结束
    t.Join();       

    
    //6.主线程退出
    return 0;  
}





