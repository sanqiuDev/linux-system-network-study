#ifndef __LOG_HPP__  // 头文件保护宏：防止头文件被重复包含，避免编译错误
#define __LOG_HPP__
 
#include <iostream>         // 提供 ---> std::cout，控制台日志输出
#include <ctime>            // 提供 ---> time、localtime_r，生成日志时间戳
#include <unistd.h>         // 提供 ---> getpid函数获取当前进程ID
#include <cstdio>           // 提供 ---> snprintf等格式化输出函数
#include <string>           // 提供 ---> std::string字符串类
#include <sstream>          // 提供 ---> std::stringstream，用于日志内容拼接

#include <filesystem>       // C++17标准库 ---> 用于文件路径操作（创建目录、判断路径存在）
#include <fstream>          // 提供 ---> std::ofstream，用于文件日志写入

#include <memory>           // 提供 ---> std::unique_ptr，智能指针管理动态资源（避免内存泄漏）


#include "Mutex.hpp"        // 包含自定义Mutex类头文件（日志写入时的线程安全保护）

//1.使用MutexModule命名空间 ---> 简化Mutex类和LockGuard的调用
using namespace MutexModule; 


// 日志模块命名空间 ---> 封装所有日志相关功能，避免命名冲突，提升代码模块化
namespace LogModule
{
    /* ===================================== 日志刷新策略（策略模式）===================================== */
    /* 关于日志刷新策略（策略模式）的说明：
    *        设计：采用"策略模式"，通过多态实现不同的日志输出方式（控制台/文件）
    *        优势：后续可扩展新策略（如网络日志），无需修改核心日志生成逻辑，符合开闭原则
    */

    //1.日志刷新策略基类（抽象类）---> 定义所有策略的统一接口
    class LogStrategy
    {
    public:
        //1.虚析构函数 ---> 确保子类对象被正确销毁（多态场景必备）
        ~LogStrategy() = default;  
        
        //2.纯虚函数 ---> 日志同步接口（子类必须实现具体的输出逻辑）
        virtual void SyncLog(const std::string &message) = 0; //参数：message - 完整的日志字符串（已拼接时间戳、等级等信息）
    };


    //2.控制台日志策略（子类）---> 实现日志输出到控制台
    const std::string gsep = "\r\n";   // 日志换行符（Windows风格，回车+换行）

    class ConsoleLogStrategy : public LogStrategy
    {
    private:
        //1.互斥锁 ---> 保护控制台输出的原子性（多线程并发写日志时避免内容错乱）
        Mutex _mutex;

    public:
        //1.“构造函数” ---> 无额外初始化逻辑
        ConsoleLogStrategy() {}  

        //2.“析构函数” ---> 无动态资源需释放
        ~ConsoleLogStrategy() {}  

        //3.“重写基类接口” ---> 将日志输出到控制台
        void SyncLog(const std::string &message) override
        {
            //3.1：RAII风格加锁：确保多线程下日志输出不混乱
            LockGuard lockguard(_mutex); 

            //3.2：控制台输出日志内容+换行符
            std::cout << message << gsep; 
        }
    };

    //3.文件日志策略（子类）---> 实现日志写入到指定文件
    const std::string defaultpath = "./log";  // 日志文件默认存储路径（当前目录下的log文件夹）
    const std::string defaultfile = "my.log"; // 日志文件默认名称

    class FileLogStrategy : public LogStrategy
    {
    private:
        //1.日志文件存储路径
        //2.日志文件名
        //3.互斥锁：保护文件写入的原子性（避免多线程并发写导致文件损坏）

        std::string _path; 
        std::string _file;  
        Mutex _mutex;     

    public:
        //1.“构造函数” ---> 初始化日志路径和文件名，若路径不存在则创建
        FileLogStrategy(const std::string &path = defaultpath, const std::string &file = defaultfile)
            : _path(path),
              _file(file)
        {
            //1.1：加锁：避免多线程同时创建目录导致冲突
            LockGuard lockguard(_mutex); 

            //1.2：检查日志路径是否已存在，存在则直接返回
            if (std::filesystem::exists(_path))
            {
                return;
            }

            //1.3：路径不存在时创建目录（递归创建多级目录）
            try
            {
                std::filesystem::create_directories(_path);  // C++17接口，递归创建目录
            }
            catch (const std::filesystem::filesystem_error &e)  // 捕获目录创建失败的异常
            {
                std::cerr << e.what() << '\n';  // 输出异常信息（如权限不足）
            }
        }
        
        //2.“析构函数” ---> 无动态资源需释放
        ~FileLogStrategy() {}  

        //3.“重写基类接口” ---> 将日志写入文件
        void SyncLog(const std::string &message) override
        {
            //3.1：加锁：确保多线程下文件写入不冲突
            LockGuard lockguard(_mutex);  

            //3.2：拼接完整的日志文件路径（处理路径末尾是否带'/'的情况）
            std::string filename = _path + (_path.back() == '/' ? "" : "/") + _file;

            //3.3：以"追加模式"打开文件（ios::app：写入时追加到文件末尾，不覆盖原有内容）
            std::ofstream out(filename, std::ios::app);
            if (!out.is_open())  
            {
                return;
            }

            //3.4：将日志内容写入文件并换行
            out << message << gsep;  
            
            ///3.5：关闭文件流（释放资源）
            out.close();             
        }
    };




    /* ===================================== 日志核心组件（日志生成+策略调度）===================================== */
    //1.获取时间戳 ---> 生成当前时间的格式化字符串（格式：年-月-日 时:分:秒）
    std::string GetTimeStamp()
    {
        //1.1：获取当前系统时间（秒级时间戳）
        time_t curr = time(nullptr);  

        //1.2：存储本地时间的结构体
        struct tm curr_tm;            
        //1.3：线程安全的本地时间转换（避免localtime的线程不安全问题）
        localtime_r(&curr, &curr_tm);  

        //1.4：格式化时间 ---> 年（tm_year+1900，因tm_year从1900开始计数）、月（tm_mon+1，因月份从0开始）
        char timebuffer[128];  
        snprintf(timebuffer, sizeof(timebuffer),"%4d-%02d-%02d %02d:%02d:%02d",
            curr_tm.tm_year + 1900,
            curr_tm.tm_mon + 1,
            curr_tm.tm_mday,
            curr_tm.tm_hour,
            curr_tm.tm_min,
            curr_tm.tm_sec
        );

        //1.5：返回格式化的时间字符串
        return timebuffer;  
    }
    
    //2.日志等级枚举 ---> 定义不同的日志优先级（从低到高）
    enum class LogLevel
    {
        DEBUG,   // 调试日志：开发阶段使用，打印详细调试信息
        INFO,    // 信息日志：正常运行状态信息（如：程序启动、模块加载）
        WARNING, // 警告日志：非致命错误，可能影响功能但不导致程序退出
        ERROR,   // 错误日志：致命错误，功能异常但程序可继续运行
        FATAL    // 严重错误：导致程序无法继续运行的致命问题
    };

    //3.日志等级转字符串 ---> 将枚举类型转换为可读的字符串（如：LogLevel::DEBUG → "DEBUG"）
    std::string LevelToStr(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";  // 异常情况：返回未知等级
        }
    }



    //4.日志器核心类 ---> 负责日志内容生成、日志策略管理（选择控制台/文件输出）
    class Logger
    {
    private:
        //1.日志刷新策略智能指针 ---> 管理LogStrategy子类对象（控制台/文件策略）
        std::unique_ptr<LogStrategy> _fflush_strategy; // 自动释放动态资源，避免内存泄漏，支持动态切换策略

    public:
        // ------------------------------- 日志策略管理（选择控制台/文件输出）------------------------------- //
        //4.1：“构造函数” ---> 默认启用控制台日志策略
        Logger()
        {
            EnableConsoleLogStrategy();
        }
         
        //4.2：“析构函数” ---> std::unique_ptr自动销毁策略对象，无需手动释放
        ~Logger() {}  

        //4.3：“启用控制台日志策略” ---> 创建ConsoleLogStrategy对象，通过智能指针管理
        void EnableConsoleLogStrategy()
        {
            _fflush_strategy = std::make_unique<ConsoleLogStrategy>();
        }

        //4.4：“启用文件日志策略” ---> 创建FileLogStrategy对象，通过智能指针管理
        void EnableFileLogStrategy()
        {
            _fflush_strategy = std::make_unique<FileLogStrategy>();
        }


        // ------------------------------- 日志消息类（辅助生成日志）------------------------------- //
        //4.5：内部类：封装单条日志的生成逻辑，支持流式拼接（如LOG(INFO) << "hello" << 123）
        class LogMessage
        {
        private:
            //1.日志生成时间戳
            //2.日志等级
            //3.完整的日志内容（固定前缀+可变内容）
            std::string _curr_time; 
            LogLevel _log_level;        
            std::string _log_info;   

            //4.进程ID
            //5.源文件名
            //6.行号
            pid_t _pid;           
            std::string _src_name;
            int _line_number;       

            //7.Logger对象引用（用于调用刷新策略）
            Logger &_logger;       


        public:
            //1.“构造函数” ---> 初始化日志的固定部分
            LogMessage(LogLevel &log_level, std::string &src_name, int line_number, Logger &logger)
                : _curr_time(GetTimeStamp()),  // 日志生成时间戳
                  _log_level(log_level),       // 日志等级

                  _pid(getpid()),              // 当前进程ID（区分多进程日志）
                  _src_name(src_name),         // 源文件名（通过__FILE__宏传入）
                  _line_number(line_number),   // 行号（通过__LINE__宏传入）

                  _logger(logger)              // Logger对象引用（用于调用刷新策略）
            {
                //1.定义stringstream类型的对象
                std::stringstream ss;

                //2.拼接日志的固定前缀（时间戳+等级+进程ID+文件名+行号）
                ss << "[" << _curr_time << "] "             // 时间戳
                   << "[" << LevelToStr(_log_level) << "] " // 日志等级
                   << "[" << _pid << "] "                   // 进程ID
                   << "[" << _src_name << "] "              // 源文件名
                   << "[" << _line_number << "] "           // 行号
                   << "- ";                                 // 分隔符

                //3.存储拼接后的固定前缀
                _log_info = ss.str();  
            }

            //2.“析构函数” ---> 自动触发日志刷新（核心！）
            ~LogMessage() // 原理：LogMessage是局部临时对象，生命周期结束时调用析构函数，自动写入日志
            {
                if (_logger._fflush_strategy)
                {
                    _logger._fflush_strategy->SyncLog(_log_info); // 若刷新策略已初始化，调用策略的SyncLog方法写入日志
                }
            }

            //3.“流式运算符重载” ---> 支持拼接任意类型的日志内容（如：字符串、数字、浮点数）
            template <typename T> //模板参数T：支持任意可被std::stringstream输出的类型
            LogMessage &operator<<(const T &info)
            {
                //1.定义stringstream类型的对象
                std::stringstream ss;

                //2.将传入的内容转换为字符串
                ss << info;  

                //3.拼接到日志完整内容中
                _log_info += ss.str(); 

                //4.返回自身引用，支持链式调用（如a << b << c）
                return *this;  
            }

        };

        //4.6：“函数调用运算符重载” ---> 创建LogMessage对象（返回临时对象，触发日志生成）
        LogMessage operator()(LogLevel level, std::string name, int line)
        {
            return LogMessage(level, name, line, *this);
        }
        /* 参数说明：
        *      1. level：日志等级
        *      2. name：源文件名（__FILE__）
        *      3. line：行号（__LINE__）
        *    返回值：LogMessage临时对象（生命周期为当前语句，析构时自动写入日志）
        */
    };





    /* ===================================== 日志宏定义（简化用户调用）===================================== */
    //1.全局日志对象 ---> 整个程序共享一个Logger实例，简化日志使用（无需手动创建）
    Logger logger;

    /*2.核心宏：封装日志等级、文件名、行号的传递，用户直接使用LOG(等级) << 内容
        __FILE__：预编译宏，自动替换为当前源文件名（字符串）
        __LINE__：预编译宏，自动替换为当前代码行号（整数）
    */
    #define LOG(level) logger(level, __FILE__, __LINE__)

    //3.启用控制台日志的宏 ---> 用户调用Enable_Console_Log_Strategy()即可切换策略
    #define Enable_Console_Log_Strategy() logger.EnableConsoleLogStrategy()

    //4.启用文件日志的宏 ---> 用户调用Enable_File_Log_Strategy()即可切换策略
    #define Enable_File_Log_Strategy() logger.EnableFileLogStrategy()
}

#endif  // 头文件保护宏结束