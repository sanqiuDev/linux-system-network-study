//尝试给新线程传递任意类型的参数！！

#include <iostream>
#include <unistd.h>
#include <pthread.h>

/*------------------------------------辅助类------------------------------------*/
//1.定义Taskt类，用于封装线程执行任务
class Task
{
private:
    //1.存储任务的两个整数参数
    int _a;
    int _b;

public:
    //1.“构造函数” ---> 初始化Task对象的两个私有成员变量_a和_b
    Task(int a, int b) : _a(a), _b(b) {}
    //2.“析构函数” ---> 空实现因为没有动态分配的资源需要释放
    ~Task() {}
    
    //3.执行任务的成员函数返回_a和_b的和
    int Execute()
    {
        return _a + _b;
    }
};


//2.定义Result类，用于封装线程执行结果
class Result
{
private:
    //1.存储结果的私有成员变量
    int _result;

public:
    //1.构造函数
    Result(int result) : _result(result){}
    //2.析构函数
    ~Result() {}

    //3.获取结果值的成员函数
    int GetResult() { return _result; }
};


/*------------------------------------新线程执行函数------------------------------------*/
void *routine(void *args)
{
    //第一步：将传入的void*参数转换为Task*类型
    Task *t = static_cast<Task*>(args);

    sleep(1); // 线程休眠1秒（模拟任务执行前的准备时间）

    //第二步：执行Task的Execute方法，并将结果封装到Result对象中，在堆上创建该对象
    Result *res = new Result(t->Execute());

    sleep(1); // 线程休眠1秒（模拟任务执行后的耗时操作）
    
    //第三步：返回Result对象的地址（作为线程的退出返回值）
    return (void*)res;
}


/*------------------------------------主函数------------------------------------*/
int main()
{
    /*------------------第一步：准备新线程------------------*/
    pthread_t tid;

    /*------------------第二步：创建新线程------------------*/
    //1.在堆上创建Task对象
    Task *t = new Task(10, 20); // 构造参数为10和20

    //2.使用pthread_create系统调用
    pthread_create(&tid, nullptr, routine, t);

    
    /*------------------第三步：回收新线程------------------*/
    //1.定义Result*指针 ---> 用于接收线程的退出返回值
    Result* ret = nullptr;

    //2.等待新线程执行完毕（阻塞主线程）
    pthread_join(tid, (void**)&ret); //注意：(void**)&ret - 输出参数，接收routine函数返回的Result*指针（需强制类型转换）
    
    /*------------------第四步：打印新线程------------------*/
    //1.获取新线程执行结果
    int n = ret->GetResult();

    //2.打印新线程的执行结果
    std::cout << "新线程结束，运行结果: " << n << std::endl;


    delete t;
    delete ret;
    return 0;
}