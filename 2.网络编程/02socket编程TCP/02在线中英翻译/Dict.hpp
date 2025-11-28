#pragma once  

// 标准库头文件包含
#include <iostream>         // 提供“标准输入输出”
#include <fstream>          // 提供“文件输入输出流” ---> 用于读取字典文件
#include <string>           // 提供“字符串处理” ---> 存储单词、翻译结果等
#include <unordered_map>    // 提供“哈希表容器” ---> 存储英文-中文的映射关系，查询效率O(1)

// 自定义头文件包含
#include "Log.hpp"          // 提供“日志模块” ---> 打印加载字典、翻译过程的日志
#include "InetAddr.hpp"     // 提供“网络地址封装类” ---> 获取客户端IP和端口，用于日志记录

/*------------------------------------------------常量定义------------------------------------------------*/
//1.默认字典文件路径（若创建Dict对象时未指定路径，使用此默认值）
const std::string defaultdict = "./dictionary.txt"; 

//2.字典文件中的分隔符（用于分割英文单词和中文翻译，格式如"apple: 苹果"）
const std::string sep = ": "; 

/*------------------------------------------------引入命令空间------------------------------------------------*/
//1.引入日志模块命名空间 ---> 简化LOG宏的调用（如：LOG(DEBUG)、LOG(WARNING)）
using namespace LogModule;


/*------------------------------------------------字典翻译类------------------------------------------------*/
// 字典翻译类 ---> 封装字典加载、单词翻译功能，支持从文件读取中英文映射关系
class Dict
{
private:
    //1.字典文件的路径 
    //2.英文-中文映射的哈希表

    std::string _dict_path; 
    std::unordered_map<std::string, std::string> _dict;

public:
    //1.“构造函数” ---> 初始化字典文件路径
    Dict(const std::string &path = defaultdict) : _dict_path(path) 
    {
        //注意：初始化时仅保存文件路径，实际加载字典的操作在LoadDict()中执行（延迟加载）
    }

    //2.“析构函数” ---> 空实现（unordered_map和string会自动析构，无需手动释放资源）
    ~Dict() {}

    //3.“加载字典文件” ---> 从指定路径读取文件，解析每一行的英文-中文映射，存入哈希表
    bool LoadDict()
    {
        //1.打开字典文件（只读模式）
        std::ifstream in(_dict_path);
        if (!in.is_open())  
        {
            LOG(LogLevel::DEBUG) << "打开字典: " << _dict_path << " 错误";  
            return false;
        }

        //2.逐行读取文件内容
        std::string line; 
        while (std::getline(in, line))  // 循环读取直到文件末尾
        {
            //2.1：查找分隔符的位置
            auto pos = line.find(sep);  
            if (pos == std::string::npos)  // 未找到分隔符（行格式错误）
            {
                LOG(LogLevel::WARNING) << "解析: " << line << " 失败";  
                continue;  // 跳过当前行，继续处理下一行
            }

            //2.2：分割英文单词和中文翻译
            std::string english = line.substr(0, pos);            // 分隔符前的子串：英文单词
            std::string chinese = line.substr(pos + sep.size());  // 分隔符后的子串：中文翻译

            //2.3：校验分割结果（避免空单词或空翻译）
            if (english.empty() || chinese.empty())
            {
                LOG(LogLevel::WARNING) << "没有有效内容: " << line;  
                continue;  // 跳过无效行
            }

            //2.4：将英文-中文映射存入哈希表（unordered_map支持快速查询）
            _dict.insert(std::make_pair(english, chinese));
            LOG(LogLevel::DEBUG) << "加载: " << line;  // 打印调试日志（记录成功加载的条目）
        }

        //3.关闭文件（释放文件资源）
        in.close();

        //4.字典加载成功
        return true; 
    }

    //4.“单词翻译” ---> 根据输入的英文单词，查询对应的中文翻译
    std::string Translate(const std::string &word, InetAddr &client)
    {
        //1.在哈希表中查找单词（O(1)时间复杂度）
        auto iter = _dict.find(word);

        //2.未找到该单词，打印调试日志并返回未找到的标识
        if (iter == _dict.end()) 
        {
            LOG(LogLevel::DEBUG) << "进入到了翻译模块, [" << client.Ip() << " : " << client.Port() << "]# " << word << "->None";
            return "None";  
        }

        //2.找到单词，打印调试日志并返回中文翻译
        LOG(LogLevel::DEBUG) << "进入到了翻译模块, [" << client.Ip() << " : " << client.Port() << "]# " << word << "->" << iter->second;
        return iter->second; 
    }
};

