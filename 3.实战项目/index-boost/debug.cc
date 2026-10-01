#include <iostream>
#include <string>
#include <cstdio>
#include <cstring>

#include "searcher.hpp"   // 引入搜索器核心头文件（包含Searcher类的定义：索引构建、搜索逻辑）

// 全局常量：指定预处理后的原始文档数据文件路径（parser模块输出的raw.txt）
const std::string input = "data/raw_html/raw.txt"; // 该文件存储了所有HTML解析后的结构化数据（标题\3内容\3URL）

// 主函数：测试Searcher模块的核心入口
int main()
{
    // ========== 1. 初始化搜索器实例 ==========
    //1.动态创建Searcher对象（堆内存），避免栈内存溢出（索引数据量大）
    ns_searcher::Searcher *search = new ns_searcher::Searcher();

    //2.初始化搜索器：加载raw.txt文件，构建正排索引+倒排索引
    search->InitSearcher(input);     // 这是搜索的前置条件，索引构建完成后才能处理搜索请求

    // ========== 2. 定义变量存储用户输入和搜索结果 ==========
    //3.
    std::string query;          // 存储用户输入的搜索关键词
    std::string json_string;    // 存储搜索结果的JSON格式字符串
    char buffer[1024];          // 临时缓冲区：接收用户输入（兼容C风格字符串）

    // ========== 3. 循环接收用户搜索请求 ==========
    //4.死循环：持续等待用户输入，直到手动终止程序（Ctrl+C）
    while(true)
    {
        //4.1：提示用户输入搜索关键词
        std::cout << "Please Enter You Search Query# ";
        
        //4.2：读取用户输入：从标准输入(stdin)读取最多1023个字符到buffer（留1位给'\0'）
        fgets(buffer, sizeof(buffer)-1, stdin); // fgets会读取换行符('\n')，因此后续需要处理
        
        //4.3：处理fgets读取的换行符：将末尾的'\n'替换为'\0'（C风格字符串结束符）
        buffer[strlen(buffer)-1] = 0; // 例如用户输入"Boost\n"，处理后变为"Boost\0"，避免关键词包含换行符
        
        //4.4：将C风格字符串转换为C++ string，方便后续调用Search接口
        query = buffer;
        
        // ========== 4. 执行搜索并输出结果 ==========
        //4.5：调用Searcher的核心接口：处理搜索关键词，生成JSON格式结果
        search->Search(query, &json_string); // 参数1：用户搜索关键词；参数2：输出参数，存储JSON结果
        
        //4.6：打印JSON格式的搜索结果（前端可直接解析该字符串）
        std::cout << json_string << std::endl;
    }

    //5.死循环不会执行到这里，实际工程中可添加退出逻辑（如输入"quit"退出）
    return 0;
}