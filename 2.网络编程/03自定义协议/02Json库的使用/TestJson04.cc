//使用JsonCpp库进行序列化和反序列化

#include <iostream>             // 包含“标准输入输出头文件” ---> 用于打印JSON序列化结果
#include <string>               // 包含“字符串处理头文件” ---> 用于存储JSON序列化后的字符串
#include <jsoncpp/json/json.h>  // 包含“JsonCpp库的核心头文件” ---> 用于JSON数据的构建与序列化

#include <sstream>              // 包含“字符串流头文件” ---> 用于内存中的数据读写
#include <memory>               // 包含“智能指针头文件” ---> 用于管理StreamWriter对象


int main()
{
    // ========== JSON序列化：将Json::Value对象转换为字符串 ==========
    //1.创建Json::Value对象
    Json::Value root;
    //2.添加键值对
    root["name"] = "张三";  // 添加字符串类型键值对
    root["sex"] = "男";     // 添加字符串类型键值对
    root["age"] = 18;       // 添加整数类型键值对

    //3.使用StreamWriterBuilder（JsonCpp推荐的序列化方式，替代老式Writer）
    Json::StreamWriterBuilder sbuilder;  // 构建StreamWriter的工具类
    //4.创建StreamWriter对象（智能指针自动管理内存，避免内存泄漏）
    std::unique_ptr<Json::StreamWriter> writer(sbuilder.newStreamWriter());

    //5.定义字符串流，作为序列化的输出目标（内存中存储结果）
    std::stringstream ss;  


    //6.执行序列化 ---> 将Json::Value对象写入字符串流
    writer->write(root, &ss);

    //7.从字符串流中提取序列化后的字符串
    std::string s = ss.str();  
    std::cout << s << std::endl;  


    // ========== JSON反序列化：将JSON字符串转换为Json::Value对象 ==========
    //1.定义待解析的JSON格式字符串（模拟从网络/文件读取的JSON数据）
    std::string json_string = "{\"name\":\"张三\", \"age\":30, \"city\":\"北京\"}";

    //2.准备反序列化的核心对象
    //2.1：存储反序列化后的JSON数据（键值对结构）
    Json::Value root;          
    //2.2：用于解析JSON字符串的工具类 
    Json::Reader reader;       

    //3.执行反序列化 ---> 将JSON字符串解析为Json::Value对象
    bool ok = reader.parse(json_string, root);
    (void)ok;  // 消除未使用变量警告

    //4.从Json::Value对象中提取数据（需与JSON字符串中的键对应）
    std::string name = root["name"].asString();  // 提取字符串类型的值（键"name"）
    int age = root["age"].asInt();               // 提取整数类型的值（键"age"）
    std::string city = root["city"].asString();  // 提取字符串类型的值（键"city"）

    //5.打印解析结果（验证反序列化是否成功）
    std::cout << name << std::endl;  // 输出：张三
    std::cout << age << std::endl;   // 输出：30
    std::cout << city << std::endl;  // 输出：北京


    return 0;
}