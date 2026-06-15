#pragma once  

// 标准库头文件包含
#include <iostream>         // 提供 ---> 标准输入输出功能
#include <string>           // 提供 ---> std::string字符串类（存储命令、执行结果等文本信息）
#include <cstdio>           // 提供 ---> popen/pclose函数（用于执行系统命令并捕获输出）
#include <set>              // 提供 ---> std::set容器（存储安全命令白名单，支持快速查找）
 
// 自定义头文件包含
#include "Command.hpp"      // 包含 ---> 命令执行类自身头文件（此处为自包含，确保头文件独立可编译）
#include "InetAddr.hpp"     // 包含 ---> 网络地址封装类（获取客户端IP和端口，用于日志记录）
#include "Log.hpp"          // 包含 ---> 日志模块头文件（打印命令执行日志，便于调试和审计）

//1.引入日志模块命名空间，简化LOG宏的调用
using namespace LogModule;

// 命令执行类：封装系统命令的安全执行逻辑，支持白名单过滤（防止恶意命令注入）
class Command
{
private:
    //1.安全命令白名单 ---> 存储所有允许执行的系统命令（std::set容器确保命令唯一，支持快速查找）
    std::set<std::string> _WhiteListCommands;

public:
    //1.“构造函数” ---> 初始化安全命令白名单（仅允许执行白名单内的命令，保障服务器安全）
    Command()
    {
        _WhiteListCommands.insert("ls");             // 列出 -> 当前目录文件
        _WhiteListCommands.insert("pwd");            // 显示 -> 当前工作目录
        _WhiteListCommands.insert("ls -l");          // 列出 -> 文件信息（含权限、大小等）
        _WhiteListCommands.insert("touch haha.txt"); // 创建 -> 名为haha.txt的空文件
        _WhiteListCommands.insert("who");            // 显示 -> 当前登录系统的用户
        _WhiteListCommands.insert("whoami");         // 显示 -> 当前执行命令的用户身份

        //注意：
        //  1. 向白名单集合中插入合法命令（严格字符串匹配，包括参数）
        //  2. 白名单需精准配置，避免包含危险命令（如：rm、sudo、sh等）
    }
    
    //2.“析构函数” 
    ~Command() {}

    //3.“命令安全校验” ---> 检查输入命令是否在白名单内（核心安全逻辑）
    bool IsSafeCommand(const std::string &cmd)
    {
        //1.调用std::set的find方法查找命令
        auto iterator = _WhiteListCommands.find(cmd); // set容器内部有序存储，查找效率O(log n)

        //2.若迭代器未指向set末尾，说明命令存在于白名单，返回true；否则返回false
        return iterator != _WhiteListCommands.end();
    }

    //4.“命令执行核心接口” ---> 校验命令合法性→执行合法命令→返回执行结果
    std::string Execute(const std::string &cmd, InetAddr &addr)
    {
        //1.校验命令合法性（白名单过滤）
        if(!IsSafeCommand(cmd))
        {
            return std::string("该命令不允许执行");  
        }

        //2.获取客户端地址信息（IP:端口），用于日志审计（追踪命令执行者）
        std::string who = addr.StringAddr();

        //3.执行合法命令（通过popen函数创建子进程执行系统命令）
        FILE *fp = popen(cmd.c_str(), "r"); // 创建管道，执行shell命令，并返回文件指针用于读取命令输出
        if(fp == nullptr)  
        {
            return std::string("你要执行的命令不存在: ") + cmd;  
        }

        //4.读取命令执行结果（从popen返回的文件指针中读取输出）
        //4.1：存储命令执行结果的字符串
        std::string res;     
        //4.2：读取缓冲区（每次读取一行命令输出）
        char line[1024];     
        //4.3：从文件指针fp中读取一行数据到line缓冲区，最多读取sizeof(line)-1字节
        while(fgets(line, sizeof(line), fp))
        {
            res += line;  
        }

        //5.关闭文件指针，释放资源（必须调用pclose，否则会导致子进程僵尸）
        pclose(fp);

        //6.构造最终结果（包含命令执行者和执行结果）
        std::string result = who + " execute done, result is: \n" + res;
        LOG(LogLevel::DEBUG) << result; 

        //7.返回执行结果给客户端
        return result;
    }

};