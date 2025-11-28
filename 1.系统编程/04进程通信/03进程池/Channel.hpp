#pragma once 

/*-------------------------------------------头文件-------------------------------------------*/
#include <iostream>
#include <cstdlib>     // 提供“标准库函数” ---> exit
#include <vector>      // 提供“vector容器” ---> 用于管理管道集合
#include <unistd.h>    // 提供“系统调用” ---> pipe、fork、close、read、write等
#include <sys/wait.h>  // 提供“waitpid函数” ---> 用于回收子进程

/*-------------------------------------------信道类（站在父进程的角度去看：“信道应该是什么样的”）-------------------------------------------*/
// 管道类：封装父进程与子进程间的通信管道写端及子进程ID，负责进程间消息发送
class Channel
{
private:
    //1.管道写端文件描述符（父进程用于向子进程对应的管道写消息）
    //2.管道对应的子进程ID（父进程用于确定向哪个子进程写的消息）
    //3.信道唯一名称（格式：channel-wfd-subid）---> 注意：这个私有成员只是为了方便我们打印消息使用

    int _wfd;          
    pid_t _subid;      
    std::string _name;

public:
    //1.“构造函数” ---> 初始化管道写端文件描述符和子进程ID，并生成唯一信道名称
    Channel(int wfd, pid_t subid) : _wfd(wfd), _subid(subid)
    {
        _name = "channel-" + std::to_string(_wfd) + "-" + std::to_string(_subid);
    }
    //2.“析构函数” ---> 默认不做额外操作，关闭文件描述符由显式调用Close()处理
    ~Channel()
    {
    }

    //3.“向子进程发送任务码”
    void Send(int code)
    {
        //3.1：写入任务码（int类型，固定4字节）
        int n = write(_wfd, &code, sizeof(code));

        //3.2：避免未使用变量的编译警告
        (void)n; 
    }

    
    //4.“关闭管道写端” ---> 通知子进程不再有任务
    void Close()
    {
        close(_wfd);
    }
    
    
    //5.“等待子进程退出并回收资源”
    void Wait()
    {
        //5.1：阻塞等待子进程结束
        pid_t rid = waitpid(_subid, nullptr, 0); 

        //5.2：避免未使用变量的编译警告
        (void)rid;                                
    }
    

    //6.“获取信道名称” ---> 用于调试和日志
    std::string Name() { return _name; }
};

/*-------------------------------------------信道管理器类（站在父进程的角度去看：“信道应该怎么进行管理”）-------------------------------------------*/
// 管道管理器：管理所有父-子进程通信信道，负责子进程选择、任务派发及资源回收
class ChannelManager
{
private:
    //1.存储所有信道的容器
    //2.下一个待选择的信道索引 ---> 用于轮询

    std::vector<Channel> _channels; 
    int _next;                      

public:
    //1.“构造函数” ---> 初始化下一个待选择的信道索引
    ChannelManager() : _next(0)
    {
    }
    
    //2.“析构函数” 
    ~ChannelManager() {}

    //3.“向管理器中添加一个新管道”（存储管道写端和对应子进程ID）
    void Insert(int wfd, pid_t subid)
    {
        _channels.emplace_back(wfd, subid); //注意：直接在vector末尾构造新Channel对象（避免拷贝，效率更高）

        /* 或者使用下面的这种方式进行添加新管道
        
         Channel c(wfd, subid);
         _channels.push_back(std::move(c));

        */
    }

    //4.“选择下一个信道” ---> 轮询策略：按顺序循环选择子进程，实现简单负载均衡
    Channel &Select()
    {
        //4.1：获取当前索引对应的信道
        auto &c = _channels[_next]; 
        
        //4.2：索引递增
        _next++;                    

        //4.3：取模实现循环
        _next %= _channels.size();  
        return c;
    }

    //5.“打印所有信道信息”（调试用）
    void PrintChannel()
    {
        for (auto &channel : _channels)
        {
            std::cout << channel.Name() << std::endl;
        }
    }
    /*--------------------------------------第一代的写法：不严谨的写法--------------------------------------
    //6.“关闭所有信道的写端” ---> 通知所有子进程停止等待任务
    void StopSubProcess()
    {
        for (auto &channel : _channels)
        {
            channel.Close();
            std::cout << "关闭: " << channel.Name() << std::endl;
        }
    }

    //7.“等待所有子进程退出并回收资源”
    void WaitSubProcess()
    {
        for (auto &channel : _channels)
        {
            channel.Wait();
            std::cout << "回收: " << channel.Name() << std::endl;
        }
    }
    ---------------------------------------------------------------------------------------------*/
    
    /*--------------------------------------第二代的写法：同时使用了的方案一解决了bug--------------------------------------*/
    //6.“关闭所有信道的写端，并回收子进程的资源” 
    //void CloseAndWait()
    //{
        /* 注意：使用下面的方法会卡bug
	    for (auto& channel : _channels)
	    {
		    channel.Close();
		    std::cout << "关闭: " << channel.Name() << std::endl;

		    channel.Wait();
		    std::cout << "回收: " << channel.Name() << std::endl;
	    }
        */

        /* 有更好的解决方案2了，这个就注释掉了
        // 解决方案1：从后往前关闭写端
        for (int i = _channels.size() - 1; i >= 0; i--)
        {
            _channels[i].Close();
            std::cout << "关闭: " << _channels[i].Name() << std::endl;

            _channels[i].Wait();
            std::cout << "回收: " << _channels[i].Name() << std::endl;
        }
        */  
    //}
    /*---------------------------------------------------------------------------------------------*/


    /*--------------------------------------第三代的写法：最终的决定--------------------------------------*/

    // 解决方案2：子进程创建时候关闭历史写端 ---> 出现在ProcessPool类中“子进程”执行逻辑中
    //6.“子进程创建时候关闭历史写端”
    void CloseAll()
    {
        for (auto &channel : _channels)
        {
            channel.Close();
            // std::cout << "关闭: " << channel.Name() << std::endl;
        }
    }

    //7.“关闭所有信道的写端(这里你要能理解到的就是现在的每个信道的写端都只有一个了，就是父进程所持有的那一个)，并回收子进程的资源”
    void CloseAndWait()
    {
        //注意：有了第6步，使用下面的方法不会卡bug
        for (auto& channel : _channels)
        {
            channel.Close();
            std::cout << "关闭: " << channel.Name() << std::endl;

            channel.Wait();
            std::cout << "回收: " << channel.Name() << std::endl;
        }
    }

};