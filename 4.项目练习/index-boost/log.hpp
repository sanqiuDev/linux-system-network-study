#ifndef LOG_HPP
#define LOG_HPP

#include <string>
#include <iostream>

//第一步：“定义日志宏等级”
// 普通(normal) ---> 警告(waring) ---> 调试(debug) ---> 致命(fatal)
#define NORMAL 1
#define WARING 2
#define DEBUG 3
#define FATAL 4


//第二步：“编写日志宏函数”
/* 日志宏函数一定要包含的四样东西
     1. 日志等级       ---> std::string 
     2. 日志内容       ---> std::string 
     3. 日志的源文件    ---> std::string 
     4. 日志代码行号    ---> int

    日志宏函数的内部就是一个输出语句
    std::cout
*/
void log(std::string level,std::string message,std::string file ,int line)
{
    std::cout<<"["<<level<<"]"
             <<"["<<time(nullptr)<<"]"
             <<"["<<message<<"]"
             <<"["<<file<<":"<<line<<"]"
             <<std::endl;
}


//第三步：简化宏函数的调用
#define LOG(LEVEL,MESSAGE) log(#LEVEL,MESSAGE,__FILE__,__LINE__)
/*
  最神奇的符号：#LEVEL → 字符串化运算符
    #LEVEL 作用：把宏参数变成字符串！
    例子：
        你写：LOG(NORMAL, "服务器启动成功");
        预处理后，#LEVEL 会把 NORMAL 变成字符串 "NORMAL"
        展开后变成：log("NORMAL", "服务器启动成功", __FILE__, __LINE__);
    重点：
        # 是宏专用字符串化运算符，只能在宏里用
    作用：把标识符 → 字符串字面量
*/



/*
  为什么要用宏？不能直接用函数吗？
    因为函数做不到这 3 件事：
        无法自动获取 __FILE__ 和 __LINE__
        函数获取的是函数内部的行号，不是调用处的！
        无法把参数变成字符串（#LEVEL）
        宏是编译期替换，没有运行时开销
*/
#endif 