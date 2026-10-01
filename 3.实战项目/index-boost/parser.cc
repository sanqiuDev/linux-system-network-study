#include <iostream>
#include <string>
#include <vector>
#include <fstream>               // 文件读写所需头文件
#include <boost/filesystem.hpp>  // Boost文件系统库，用于递归遍历目录

#include "util.hpp"              // 自定义工具库，包含文件读取等工具函数

// ===================== 全局常量定义 =====================
//1.原始HTML文件的根目录（存放待解析的Boost文档）
const std::string src_path = "data/input";
//2.解析后的数据输出文件路径（存储标题、内容、URL，用\3分隔）
const std::string output = "data/raw_html/raw.txt";

// ===================== 数据结构定义 =====================
/**
 * @brief 文档信息结构体，存储单个HTML文档的核心信息
 * @details 对应搜索引擎中"文档"的核心元数据，是构建索引的基础单元
 */
typedef struct DocInfo 
{
    std::string title;   // 文档标题（从<title>标签提取）
    std::string content; // 文档纯文本内容（去除所有HTML标签后的内容）
    std::string url;     // 文档对应的在线URL（拼接Boost官网路径生成）
} DocInfo_t;  // 类型别名，简化结构体使用

// ===================== 函数声明 =====================
/**
 * @brief 递归枚举指定目录下所有HTML文件
 * @param src_path 输入参数：待遍历的根目录路径
 * @param files_list 输出参数：存储所有符合条件的HTML文件路径（带完整路径）
 * @return bool 枚举成功返回true，失败返回false
 */
bool EnumFile(const std::string &src_path, std::vector<std::string> *files_list);

/**
 * @brief 解析HTML文件列表，提取每个文件的标题、内容、URL
 * @param files_list 输入参数：待解析的HTML文件路径列表
 * @param results 输出参数：存储解析后的所有文档信息
 * @return bool 解析成功返回true，失败返回false
 */
bool ParseHtml(const std::vector<std::string> &files_list, std::vector<DocInfo_t> *results);

/**
 * @brief 将解析后的文档信息写入输出文件
 * @param results 输入参数：解析完成的文档信息列表
 * @param output 输入参数：输出文件路径
 * @return bool 写入成功返回true，失败返回false
 */
bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output);

// ===================== 主函数 =====================
/**
 * @brief 程序入口函数，完成"枚举文件→解析内容→保存结果"的核心流程
 * @return int 程序退出码：0成功，1枚举文件失败，2解析HTML失败，3保存结果失败
 */
int main()
{
    //1.递归枚举src_path下所有HTML文件，存入files_list
    std::vector<std::string> files_list; //存储所有待解析的HTML文件路径
    if(!EnumFile(src_path, &files_list))
    {
        std::cerr << "enum file name error!" << std::endl;
        return 1;
    }

    //2.解析每个HTML文件，提取标题、内容、URL
    std::vector<DocInfo_t> results; //存储解析后的所有文档信息
    if(!ParseHtml(files_list, &results))
    {
        std::cerr << "parse html error" << std::endl;
        return 2;
    }

    //3.将解析结果写入输出文件（用\3作为字段分隔符，\n作为文档分隔符）
    if(!SaveHtml(results, output))
    {
        std::cerr << "save html error" << std::endl;
        return 3;
    }

    return 0; // 程序正常退出
}

// ===================== 核心函数实现 =====================
bool EnumFile(const std::string &src_path, std::vector<std::string> *files_list)
{
    //1.命名空间别名，简化Boost文件系统库的使用
    namespace fs = boost::filesystem;

    //2.将字符串路径转换为Boost文件系统的path对象
    fs::path root_path(src_path);
    //3.检查根目录是否存在，不存在则直接返回失败
    if(!fs::exists(root_path))
    {
        std::cerr << src_path << " not exists" << std::endl;
        return false;
    }

    //4.递归目录迭代器：end为迭代器结束标志（空迭代器）
    fs::recursive_directory_iterator end;
    //5.遍历根目录下所有文件/子目录（递归）
    for(fs::recursive_directory_iterator iter(root_path); iter != end; iter++)
    {
        //第一步：仅处理普通文件（排除目录、链接等）
        if(!fs::is_regular_file(*iter)){ 
            continue;
        }
        //第二步：仅处理.html后缀的文件（Boost文档格式）
        if(iter->path().extension() != ".html"){ 
            continue;
        }
        //第三步：将符合条件的文件路径（字符串格式）存入列表
        files_list->push_back(iter->path().string());
    }

    return true;
}


/**
 * @brief 静态辅助函数：从HTML文本中提取<title>标签内的标题
 * @param file 输入参数：完整的HTML文件内容
 * @param title 输出参数：提取到的标题字符串
 * @return bool 提取成功返回true，无<title>标签返回false
 */
static bool ParseTitle(const std::string &file, std::string *title)
{
    //1.查找<title>标签的起始位置
    std::size_t begin = file.find("<title>");
    if(begin == std::string::npos){ 
        return false;
    }
    //2.查找</title>标签的结束位置
    std::size_t end = file.find("</title>");
    if(end == std::string::npos){ 
        return false;
    }

    //3.偏移到<title>标签之后（跳过标签本身）
    begin += std::string("<title>").size();

    //4.边界检查：起始位置不能超过结束位置（避免无效数据）
    if(begin > end){
        return false;
    }
    //5.提取标题内容（从begin到end-begin的子串）
    *title = file.substr(begin, end - begin);
    return true;
}

/**
 * @brief 静态辅助函数：去除HTML所有标签，提取纯文本内容
 * @param file 输入参数：完整的HTML文件内容
 * @param content 输出参数：去除标签后的纯文本内容
 * @return bool 处理成功返回true（即使内容为空）
 * @details 基于简易状态机实现：
 *          - LABLE状态：当前字符在HTML标签内，跳过
 *          - CONTENT状态：当前字符在标签外，保留（替换换行符为空格）
 */
static bool ParseContent(const std::string &file, std::string *content)
{
    //1.状态机枚举：标记当前字符是否在HTML标签内
    enum status
    {
        LABLE,    // 标签内状态（跳过字符）
        CONTENT   // 内容状态（保留字符）
    };

    //1.初始状态：默认先进入标签内（HTML以<开头）
    enum status s = LABLE; 
    //2.遍历每个字符
    for(char c : file)
    {   
        switch(s)
        {
            case LABLE:
                //情况一：遇到>表示标签结束，切换到内容状态
                if(c == '>') s = CONTENT;
                break;
            case CONTENT:
                //情况二：遇到<表示新标签开始，切换到标签状态
                if(c == '<') s = LABLE;

                //情况三：替换换行符为空格（避免解析后内容换行混乱）
                else 
                {
                    if(c == '\n') c = ' ';
                    content->push_back(c); // 保留有效字符
                }
                break;
            default:
                break;
        }
    }

    return true;
}

/**
 * @brief 静态辅助函数：根据本地文件路径构建在线URL
 * @param file_path 输入参数：本地HTML文件的完整路径
 * @param url 输出参数：拼接后的在线URL
 * @return bool 构建成功返回true
 * @details URL规则：
 *          前缀：Boost官网文档根路径（1_78_0版本）
 *          后缀：本地路径去除src_path后的相对路径
 */
static bool ParseUrl(const std::string &file_path, std::string *url)
{
    //1.Boost官网文档根URL（对应1_78_0版本）
    std::string url_head = "https://www.boost.org/doc/libs/1_78_0/doc/html";

    //2.提取本地路径中src_path之后的相对路径（拼接为URL后缀）
    std::string url_tail = file_path.substr(src_path.size());

    //3.拼接完整URL
    *url = url_head + url_tail;
    return true;
}

/**
 * @brief 静态辅助函数：调试用，打印单个文档的解析结果
 * @param doc 输入参数：待打印的文档信息结构体
 */
static void ShowDoc( const DocInfo_t &doc)
{
    std::cout << "title: " << doc.title << std::endl;
    std::cout << "content: " << doc.content << std::endl;
    std::cout << "url: " << doc.url << std::endl;
}


bool ParseHtml(const std::vector<std::string> &files_list, std::vector<DocInfo_t> *results)
{
    for(const std::string &file : files_list) //遍历每个HTML文件路径
    {
        //1.读取文件内容到result字符串（调用自定义工具类）
        std::string result;
        if(!ns_util::FileUtil::ReadFile(file, &result))
        {
            std::cerr << "Read file failed: " << file << std::endl;
            continue; // 单个文件读取失败，跳过继续处理下一个
        }

        //2.提取标题
        DocInfo_t doc; // 存储当前文件的解析结果
        if(!ParseTitle(result, &doc.title)){
            std::cerr << "Parse title failed: " << file << std::endl;
            continue;
        }
        //3.提取纯文本内容：失败则跳过当前文件
        if(!ParseContent(result, &doc.content)){
            std::cerr << "Parse content failed: " << file << std::endl;
            continue;
        }
        //4.构建在线URL：失败则跳过当前文件
        if(!ParseUrl(file, &doc.url)){
            std::cerr << "Parse url failed: " << file << std::endl;
            continue;
        }

        //5.使用std::move减少拷贝（doc是局部变量，移动后失效）
        results->push_back(std::move(doc));

        // 调试：打印单个文档信息（可注释）
        // ShowDoc(doc);
        // break; // 调试时仅处理第一个文件
    }
    return !results->empty(); // 只要有一个文件解析成功，就返回true
}

/**
 * @brief 将解析后的文档信息写入输出文件
 * @param results 输入参数：解析完成的文档信息列表
 * @param output 输入参数：输出文件路径
 * @return bool 写入成功返回true，文件打开失败返回false
 * @details 输出格式：title\3content\3url\n
 *          - \3（ASCII码3）：字段分隔符（避免与文本内容冲突）
 *          - \n：文档分隔符
 *          - 二进制模式写入：避免换行符转换（Windows下\n→\r\n）
 */
bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output) //注意：这里不需要定义为指针
{
    // 定义字段分隔符（ASCII 3，非打印字符，避免与内容冲突）
#define SEP '\3'

    //1.以二进制模式打开输出文件（out：覆盖写入，binary：避免换行符转换）
    std::ofstream out(output, std::ios::out | std::ios::binary);
    if(!out.is_open()){ 
        std::cerr << "open " << output << " failed!" << std::endl;
        return false;
    }

    //2.遍历所有解析结果，逐行写入
    for(auto &item : results)
    {
        //第一步：拼接单行输出内容：title + 分隔符 + content + 分隔符 + url + 换行
        std::string out_string;
        out_string = item.title;
        out_string += SEP;
        out_string += item.content;
        out_string += SEP;
        out_string += item.url;
        out_string += '\n';

        //第二步：写入文件（c_str()获取字符串首地址，size()获取字节数）
        out.write(out_string.c_str(), out_string.size());
    }

    //3.关闭文件（避免资源泄漏）
    out.close();

    // 释放宏定义（避免作用域污染）
#undef SEP

    return true;
}



