//1.上来先做一下投文件保护吧
// #ifndef LOGGER_HPP
// #define LOGGER_HPP
/*
   ✅ 推荐你用：#define UTIL_HPP
   ❌ 绝对不要用：#define _UTIL_HPP_

   为什么不能用 _UTIL_HPP_？
   因为 C++ 标准规定：所有以下划线 _ 开头 + 大写字母 的名字，都属于【编译器 / 标准库保留】！
   最标准、最安全、最推荐的格式：文件名全大写 + 后缀替换为大写
*/
#pragma once

#include <time.h>
#include <stdio.h>

//2.写一个日志都需要什么呢？
//     日志的等级：正常inf  警告waring   调试  debug 致命fata

#define INF 0
#define DBG 1
#define ERR 2

//3. 接下搞定全局默认的日志等级
#define DEFAULT_LOG_LEVEL INF


//4. 写核心的日志函数 （只是声明），简单点说就是让日志能像printf一样使用
//日志函数实现我们选择使用宏，所以
// #define LOG(level,format,...) // 宏函数的全半个部分是这样固定的，参数就是
//1. 日志的水平，2.格式化字符串就是错误信息，3.可变参数，简单说就是：如："connect mysql failed: %s"
//do{
  //很多人会好奇就是为什么这里要使用一个do{...} while(0) 语句：
  //作用就是：将宏体封装为循环语句，保证宏在任何场景下（尤其是if/else语句中）语法正确，避免逻辑错误

  //好了来看这里面要怎么写：
    //1.日志过滤——不需要的日志直接跳过
    //2.获取当前时间 ---> 存入结构体中 ---> 格式化成人类可读时间字符串（time -> localtime -> strftime）
    //3.打印日志到控制台 (fprintf)


    //为什么要日志过滤，怎么进行日志过滤呢？
    //操作就是：if(DEFAULT_LOG_LEVEL > level) break; //如果【全局门槛】 > 【这条日志等级】 → 直接跳过不打印！
    //不过现在是设置了所有的日志的都会一条不漏的输出，但是我们还是要怎么做，因为这样日志输出更灵活了，以后可能会用到

    //怎么获取当前时间？
    //1.拿到的当前系统的秒数，其实也就是拿到当前时间的时间戳
    // time_t t = time(NULL); //注意要添加<time.h>
    //2.将秒数转化为“时分秒”的结构体
    //这个操作具体怎么实现的我们不需要操心 localtime这个函数能直接完成，只不过就是你需要将时间戳取地址传进去
    // struct tm *lt = localtime(&t); //现在的情况就是lt这个指针可以拿到此时的时分秒

    /*
        struct tm 
        {
          int tm_sec;    // 秒
          int tm_min;    // 分
          int tm_hour;   // 时
          int tm_mday;   // 日
          int tm_mon;    // 月
          int tm_year;   // 年
          int tm_wday;   // 星期几
          // ...
        };

        lt->tm_hour  // 时
        lt->tm_min   // 分
        lt->tm_sec   // 秒
    */

   //所以接下来要考虑的问题是怎么将其格式化为人类可读的字符串
   //这个可以使用函数与strftime函数解决， 把时间结构体 → 格式化成你想要的时间字符串
   /*
       strftime(
          存结果的地方,    // char* buf
          最多存多少字节,  // 31
          你想要的格式,    // "%H:%M:%S"
          时间结构体指针   // lt 
       );
   */ 
  //简单点说就是：把 lt 里面的时间 → 变成 "时：分: 秒" 字符串 → 放进 buf 里
  // char buff[32]={0};
  // strftime(buff,31,"%H:%M:%S",lt); //注意双引号："%H:%M:%S"


    
  //最后解决输出问题，就是fprinf函数,fprinf(stdout,"[%s:%s:%d]" format "\n",buff,__FILE,__LINE__,##__va_ARGS__);
  //这个函数的参数安排是：
  /*
    stdout：输出到控制台（屏幕）
    中间是输出格式
        [%s:%s:%d]
            [ ]：括号包裹，好看
            %s：放时间（buf）
            %s：放文件名（FILE）
            %d：放行号（LINE）
           效果就像是这样：[15:40:22 main.cpp:10]
        format；
            这里是你自己传进来的字符串，比如："初始化成功"、"值=%d"
        "\n"
           每条日志打完自动换行。
    后面是要填充的数据：
        buf,           // 对应第一个 %s → 时间
        __FILE__,      // 对应第二个 %s → 文件名
        __LINE__,      // 对应 %d       → 行号
        ##__VA_ARGS__  // 对应你传的参数（如%d、%s等）

        ##__VA_ARGS__ 这是宏的可变参数，专门处理你传的：LOG(INF, "num=%d", 100);
  */
//}



//看了上面介绍之后我们可以自己写一下上面的日志函数，本质上就是一个打印函数
#define LOG(level,format,...) do{\
    if(level < DEFAULT_LOG_LEVEL) break;\
    time_t t = time(NULL);\
    struct tm *lt = localtime(&t);\
    char buf[32]={0};\
    strftime(buf,31,"%H:%M:%S",lt);\
    fprintf(stdout,"[%s:%s:%d]" format "\n",buf,__FILE__,__LINE__,##__VA_ARGS__);\
}while(0)

//上面的代码我说几点，就是：
//    1.首先你要记得加上：\  换行续行符
//    2.其次就是行与行之间不要空行
//    3.最后就是记得最后一行也是要加换行续行符的

//我们最后一步就是简化日志函数
#define ILOG(format,...) LOG(INF,format,##__VA_ARGS__);
#define DLOG(format,...) LOG(DBG,format,##__VA_ARGS__);
#define ELOG(format,...) LOG(ERR,format,##__VA_ARGS__);


// #endif