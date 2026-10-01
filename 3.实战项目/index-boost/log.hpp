#pragma once 

#include <iostream>  
#include <string>    
#include <ctime>     // 时间库（获取日志打印的时间戳）

// ===================== 日志级别宏定义 =====================
/**
 * @brief 日志级别宏定义（数值越大，级别越高）
 * @details 不同级别用于区分日志的重要性，便于后续过滤/分析：
 *          - NORMAL：普通日志（常规运行信息，如"索引构建成功"）
 *          - WARNING：警告日志（非致命错误，如"单个文件解析失败"）
 *          - DEBUG：调试日志（开发阶段调试信息，如"当前处理文档数"）
 *          - FATAL：致命日志（严重错误，如"字典文件加载失败"）
 */
#define NORMAL  1    // 普通级别
#define WARNING 2    // 警告级别
#define DEBUG   3    // 调试级别
#define FATAL   4    // 致命级别

// ===================== 日志打印宏封装 =====================
/**
 * @brief 日志打印宏（核心封装，简化日志调用）
 * @param LEVEL 日志级别（如NORMAL/WARNING，会被#转为字符串）
 * @param MESSAGE 日志消息（自定义字符串，如"建立索引成功"）
 * @details 宏替换规则：
 *          1. #LEVEL：将LEVEL参数转为字符串（如NORMAL→"NORMAL"）
 *          2. __FILE__：编译器内置宏，当前源文件的路径（如"src/index.cpp"）
 *          3. __LINE__：编译器内置宏，当前代码行号（整数）
 *          调用示例：LOG(NORMAL, "获取index单例成功...") → 展开为 log("NORMAL", "获取index单例成功...", __FILE__, __LINE__)
 */
#define LOG(LEVEL, MESSAGE) log(#LEVEL, MESSAGE, __FILE__, __LINE__)

// ===================== 日志核心函数 =====================
/**
 * @brief 日志打印核心函数（被LOG宏调用，不建议直接调用）
 * @param level 输入参数：日志级别字符串（如"NORMAL"）
 * @param message 输入参数：日志具体消息内容
 * @param file 输入参数：日志所在源文件路径（由__FILE__传入）
 * @param line 输入参数：日志所在代码行号（由__LINE__传入）
 * @details 日志输出格式：
 *          [级别][时间戳][消息][文件:行号]
 *          示例：[NORMAL][1741000000][获取index单例成功...][src/searcher.cpp : 25]
 */
void log(std::string level, std::string message, std::string file, int line)
{
    // 拼接并打印日志内容，各字段用[]包裹，便于后续解析
    std::cout << "[" << level << "]"                 // 日志级别
              << "[" << time(nullptr) << "]"         // 时间戳（秒级，从1970-01-01 00:00:00 UTC开始）
              << "[" << message << "]"               // 日志消息
              << "[" << file << " : " << line << "]" // 日志所在文件和行号
              << std::endl;                          // 换行，结束日志行
}