#pragma once  

#include <iostream>   // 标准输入输出（辅助调试）
#include <fstream>    // 文件流操作（读取文件内容）
#include <string>     // 字符串处理（存储文件内容、分隔符等）

// 工具类 ---> 提供文件读取、字符串分割等通用辅助功能
class Util // 注意：所有方法均为静态方法，无需实例化即可调用
{
public:
    //1.“读取文件内容” ---> 支持文本和二进制文件，如HTML、图片等
    static bool ReadFileContent(const std::string &filename, std::string *out)
    {
        /*-------------------------------【版本1：文本方式读取】-------------------------------*/
        /*-------------------------- 第一步：打开文件--------------------------*/
        std::ifstream in(filename);  // 文本模式打开文件
        if (!in.is_open()) 
        {
            return false;
        }

        /*-------------------------- 第二步：读取文本--------------------------*/
        std::string line;
        while(std::getline(in, line))  // 逐行读取文本
        {
            *out += line; 
        }

        /*-------------------------- 第三步：关闭文件--------------------------*/
        in.close();  


        /*-------------------------------【版本2：二进制方式读取】-------------------------------*/
        // （推荐，支持所有类型文件）
        // int filesize = FileSize(filename);  // 获取文件大小
        // if (filesize > 0)  // 文件存在且大小有效
        // {
        //     std::ifstream in(filename, std::ios::binary);  // 二进制模式打开文件
        //     if (!in.is_open())  // 打开失败（如文件不存在、权限不足）
        //         return false;

        //     out->resize(filesize);  // 预分配输出字符串的内存（避免多次扩容）
        //     // 读取文件内容到字符串（c_str()返回字符指针，需强转为可写指针）
        //     in.read((char*)(out->c_str()), filesize);
        //     in.close();  // 关闭文件
        // }
        // else  // 文件大小无效（<=0）或获取大小失败
        // {
        //     return false;
        // }

        // return true;  // 读取成功
    }

    //2.“从大字符串中提取一行” ---> 以指定分隔符结尾
    static bool ReadOneLine(std::string &bigstr, std::string *out, const std::string &sep)
    {
        //1.查找分隔符在大字符串中的位置
        auto pos = bigstr.find(sep);
        if (pos == std::string::npos)  
        {
            return false; // 未找到分隔符，无法提取完整行
        }

        //2.提取从开头到分隔符的内容（不包含分隔符）
        *out = bigstr.substr(0, pos);

        //3.从大字符串中删除已提取的行及分隔符（避免重复处理）
        bigstr.erase(0, pos + sep.size());
        return true;  
    }

    //3.“获取文件大小”（字节数）
    static int FileSize(const std::string &filename)
    {
        //1.二进制模式打开文件（不关心内容，仅获取大小）
        std::ifstream in(filename, std::ios::binary);
        if (!in.is_open())  
        {
            return -1;
        }

        //2.将文件指针移动到文件末尾
        in.seekg(0, in.end);

        //3.获取当前指针位置 ---> 即文件大小，单位字节
        int filesize = in.tellg();

        //4.将文件指针移回开头（不影响后续操作，好习惯）
        in.seekg(0, in.beg);

        //5.关闭文件
        in.close();  
        return filesize;
    }
};