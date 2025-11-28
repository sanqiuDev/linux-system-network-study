#include "ThreadPool.hpp"   // 包含线程池模块头文件 ---> 使用单例模式的线程池
#include "Log.hpp"          // 包含自定义日志模块头文件 ---> 用于打印日志信息
#include "Task.hpp"         // 包含任务定义头文件 ---> 包含task_t类型和Download任务函数


//1.引入日志模块命名空间 ---> 简化日志接口（如LOG宏、日志等级、日志策略函数）的调用
using namespace LogModule;
//2.引入线程池模块命名空间 ---> 简化线程池类的调用
using namespace ThreadPoolModule;

int main()
{
    //1.启用控制台日志策略
    Enable_Console_Log_Strategy();

    //2.循环提交任务：每秒提交1个下载任务到线程池
    int count = 5;
    while (count)
    {
        sleep(1);  // 休眠1秒，控制任务提交节奏

        //1.获取线程池单例实例（首次调用时创建5个工作线程并启动）
        ThreadPool<task_t>::GetInstance()->Enqueue(Download);
        /*说明：
        *  1.线程池是单例模式，GetInstance()首次调用时会自动创建实例并启动（内部调用Start()）
        *  2.调用Enqueue()方法提交任务：将Download函数（符合task_t类型）加入任务队列
        *  3.线程池会自动唤醒空闲线程处理任务（无需手动唤醒）
        */

        //2.任务计数器减1，直到提交完10个任务
        count--;   
    }

    //3.停止线程池：标记线程池为"停止状态"，唤醒所有休眠线程准备退出
    ThreadPool<task_t>::GetInstance()->Stop(); // 线程池停止后不再接收新任务，已入队的任务会继续处理 

    //4.等待所有工作线程退出：阻塞主线程，直到所有工作线程处理完剩余任务并终止
    ThreadPool<task_t>::GetInstance()->Join();

    return 0;
}