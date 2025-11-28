#include "Thread.hpp"       // 包含自定义的Thread类模板头文件（封装POSIX线程操作）
#include <unistd.h>         // 提供“Unix系统调用” ---> 如sleep函数，用于线程休眠）
#include <vector>           // 提供“vector容器” ---> 用于批量管理Thread对象

using namespace ThreadModule;  

int main()
{
    /*----------------------------------------【单线程测试】----------------------------------------*/
    
    //1.创建单个Thread对象，绑定lambda表达式作为线程函数
    Thread t([](){ //同样的这行代码中的模板中的写法是：Thread<void> t([](){
        while(true)
        {
            char name[128];
            pthread_getname_np(pthread_self(), name, sizeof(name));
            
            std::cout << "我是一个新线程: " << name << std::endl;
            sleep(1);
        }
    });

    //2.创建新线程
    t.Create();   
    //3.设置线程为分离状态，退出时自动释放资源
    t.Detach();   

    sleep(5);     // 主线程休眠5秒，让子线程运行一段时间

    //4.强制终止线程（调用pthread_cancel）
    t.Cancel();   

    sleep(5);     // 主线程再休眠5秒，观察线程终止后的状态

    //5.尝试等待线程（因之前调用Detach，join会失败并打印提示）
    t.Join();     


    /*----------------------------------------【多线程测试】----------------------------------------*/
    
    //1.定义vector容器，用于存储3个Thread对象（批量管理线程）
    std::vector<Thread> threads; 
    //注意：如果是模板的话，Thread类模板需指定参数类型，此处lambda无参数，模板参数应为void
    //所以：上面的代码在泛型编程中应该是：std::vector<Thread<void>> threads;

    //2.循环创建3个线程对象，存入vector容器
    for (int i = 0; i < 3; i++)
    {
        //2.1：调用emplace_back直接在容器中构造Thread对象（避免拷贝，更高效）
        threads.emplace_back([]()
        {
            while(true)
            {
                char name[128];  
                pthread_getname_np(pthread_self(), name, sizeof(name)); //获取当前线程的名称
                
                std::cout << "我是一个新线程: " << name << std::endl; 
                sleep(1);  
            } 
        });
    }

    //3.批量创建所有线程 ---> 遍历vector，调用每个Thread对象的Create()方法
    for (auto &thread : threads)
    {
        thread.Create();  //注意：内部调用pthread_create创建线程，执行lambda表达式中的逻辑
    }

    sleep(3);     // 主线程休眠3秒，让子线程运行一段时间

    //4.批量强制终止线程 ---> 遍历vector，调用每个Thread对象的Cancel()方法
    for (auto &thread : threads)
    {
        thread.Cancel();  
    }   

    sleep(3);     // 主线程再休眠3秒，观察线程终止后的状态

    //4.批量等待所有线程退出 ---> 遍历vector，调用每个Thread对象的Join()方法
    for (auto &thread : threads)
    {
        thread.Join();  // 内部调用pthread_join，回收线程资源，避免僵尸线程
    }

    
    
    return 0;  
}



