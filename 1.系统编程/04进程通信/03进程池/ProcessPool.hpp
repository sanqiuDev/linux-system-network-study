
#ifndef __PROCESS_POOL_HPP__
#define __PROCESS_POOL_HPP__

/*-------------------------------------------头文件-------------------------------------------*/
#include "Task.hpp"    // 包含“任务管理器”相关声明
#include "Channel.hpp" // 包含“信道管理器”相关声明

/*-------------------------------------------全局变量-------------------------------------------*/
//1.全局默认进程池大小（5个子进程）
const int gdefaultnum = 5;

/*-------------------------------------------进程池类-------------------------------------------*/
// 进程池类：管理一组子进程，负责创建子进程、派发任务、回收资源
class ProcessPool
{
private:
    //1.进程池中子进程的数量
    //2.任务管理器 ---> 存储任务函数并生成任务码
    //3.信道管理器 ---> 管理父-子进程通信

    int _process_num;  
    TaskManager _tm;   
    ChannelManager _cm; 

public: 
    //1.“构造函数” ---> 初始化进程池大小，并向任务管理器注册预设任务
    ProcessPool(int num) : _process_num(num)
    {
        //1.注册"打印日志"任务
        _tm.Register(PrintLog); 

        //2.注册"下载"任务
        _tm.Register(Download); 

        //3.注册"上传"任务
        _tm.Register(Upload);    
    }

    //2.“析构函数” ---> 默认不做额外操作，资源回收由Stop()显式处理
    ~ProcessPool()
    {    
    }

    //3.“子进程工作函数” ---> 循环从管道读端接收任务码并执行对应任务
    void Work(int rfd)
    {
        while (true)
        {
            //1.存储接收的任务码
            int code = 0; 
            //2.从管道读端读取任务码（阻塞等待，直到有数据或写端关闭）
            ssize_t n = read(rfd, &code, sizeof(code));

            //情况一：成功读取到数据
            if (n > 0)  
            {
                //3.1：数据不完整（异常情况忽略）
                if (n != sizeof(code)) 
                {
                    continue;
                }
                //3.2：打印子进程ID和收到的任务码（调试信息）
                std::cout << "子进程[" << getpid() << "]收到一个任务码: " << code << std::endl;

                //3.3：调用任务管理器执行对应任务
                _tm.Execute(code); 
            }

            //情况二：管道写端已关闭（父进程通知退出）
            else if (n == 0) 
            {
                std::cout << "子进程退出" << std::endl;
                break; 
            }

            //情况三：读操作失败（如错误信号中断）
            else 
            {
                std::cout << "读取错误" << std::endl;
                break; 
            }
        }
    }

    //4.“启动进程池” ---> 创建指定数量的子进程和通信管道，并初始化信道管理器
    bool Start()
    {
        for (int i = 0; i < _process_num; i++) //注意：这里的for循环只有我们的父进程才会跑，因为子进程被创建之后就会跳转去执行Work()函数了
        {                                      //所以说这里不用担心子进程会带来的影响
            //1.创建匿名管道
            int pipefd[2] = {0};
            int n = pipe(pipefd);
            //2.管道创建失败，返回错误
            if (n < 0) 
                return false;

            //2.创建子进程
            pid_t subid = fork();
            //3.子进程创建失败，返回错误
            if (subid < 0) 
                return false;

            /*--------------“子进程”执行逻辑--------------*/
            else if (subid == 0) 
            {
                //解决方案2.让子进程关闭它继承下来的父进程持有的管道的写端
                _cm.CloseAll();

                //4.子进程关闭管道写端（只需要读端接收任务）
                close(pipefd[1]);

                //5.进入工作循环（从读端接收任务）
                Work(pipefd[0]);

                //6.工作结束，关闭读端并退出
                close(pipefd[0]);
                
                //7.子进程退出
                exit(0);
            }
            /*--------------“父进程”执行逻辑--------------*/
            else 
            {
                //4.父进程关闭管道读端（只需要写端发送任务）
                close(pipefd[0]);

                //5.将写端和子进程ID存入信道管理器
                _cm.Insert(pipefd[1], subid);
            }
        }
        return true; 
    }


    //5.“派发任务” ---> 生成任务码并通过轮询选择子进程发送
    void Run()
    {
        //1.从任务管理器获取一个随机或预设的任务码
        int taskcode = _tm.Code();

        //2.通过信道管理器选择一个子进程
        auto &c = _cm.Select();
        std::cout << "选择了一个子进程: " << c.Name() << std::endl;

        //3.向选中的子进程发送任务码
        c.Send(taskcode);
        std::cout << "发送了一个任务码: " << taskcode << std::endl;
    }

   
    //6.“停止进程池” ---> 关闭所有通信管道，回收子进程资源
    void Stop()
    {
        /*第一版：
        //1.关闭所有管道写端（触发子进程退出）
        _cm.StopSubProcess();

        //2.等待所有子进程退出并回收
        _cm.WaitSubProcess();
        */
        
        //第二、三版：
        _cm.CloseAndWait();  //关闭所有信道的写端，并回收子进程的资源
        
    }
    
};

#endif