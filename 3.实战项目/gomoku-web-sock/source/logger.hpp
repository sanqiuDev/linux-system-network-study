// 头文件保护宏：防止该头文件被重复包含，避免出现变量/宏重定义编译错误
// 工作逻辑：如果__M_LOGGER_H__
//       1. 未被定义，则执行后续代码并定义该宏
//       2. 若已定义，直接跳过整个文件
#ifndef __M_LOGGER_H__
#define __M_LOGGER_H__

#include <stdio.h>  // 引入C标准输入输出库：提供fprintf(stdout)用于日志打印输出
#include <time.h>   // 引入C时间处理库：提供time()、localtime()、strftime()用于获取并格式化日志时间戳

/************************ 日志等级定义（宏常量） ************************/
#define INF 0   // 信息级日志（Information）：用于打印普通运行信息、流程节点提示等
#define DBG 1   // 调试级日志（Debug）：用于开发阶段调试，打印变量值、中间流程结果等
#define ERR 2   // 错误级日志（Error）：用于打印程序运行错误、异常信息（如：接口调用失败、文件打开失败等）

/************************ 全局日志过滤等级配置 ************************/
//默认日志输出等级：设置为INF（0），表示所有等级（INF、DBG、ERR）的日志都会被打印
//说明：
//  1. 若改为DBG（1），则仅打印DBG、ERR等级的日志，INF等级被过滤
//  2. 若改为ERR（2），则仅打印ERR等级的日志，INF、DBG等级被过滤
//过滤逻辑：日志等级值 大于 DEFAULT_LOG_LEVEL 的日志会被忽略
#define DEFAULT_LOG_LEVEL INF

/************************ 核心日志宏定义（核心实现） ************************/
// 通用日志宏：支持自定义日志等级、格式化输出、可变参数，封装了完整的日志打印逻辑
// 参数说明：
//  level：日志等级（INF/DBG/ERR）
//  format：日志格式化字符串（与printf格式一致，如："connect mysql failed: %s"）
//  ...：可变参数列表（对应format中的占位符，如：错误信息字符串、数值等）
//  do{...}while(0)：将宏体封装为循环语句，保证宏在任何场景下（尤其是if/else语句中）语法正确，避免逻辑错误
#define LOG(level, format, ...) do{\
    if (level<DEFAULT_LOG_LEVEL) break;\
    time_t t = time(NULL);\
    struct tm *lt = localtime(&t);\
    char buf[32] = {0};\
    strftime(buf, 31, "%H:%M:%S", lt);\
    fprintf(stdout, "[%s %s:%d] " format "\n", buf, __FILE__, __LINE__, ##__VA_ARGS__);\
}while(0)

/************************ 简化日志宏定义（便捷调用） ************************/
#define ILOG(format, ...) LOG(INF, format, ##__VA_ARGS__) // 信息级日志快捷宏：封装LOG(INF, ...)，直接调用即可打印信息级日志
#define DLOG(format, ...) LOG(DBG, format, ##__VA_ARGS__) // 调试级日志快捷宏：封装LOG(DBG, ...)，直接调用即可打印调试级日志
#define ELOG(format, ...) LOG(ERR, format, ##__VA_ARGS__) // 错误级日志快捷宏：封装LOG(ERR, ...)，直接调用即可打印错误级日志

// 结束头文件保护宏
#endif