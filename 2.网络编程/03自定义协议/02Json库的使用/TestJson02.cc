//JsonCpp库中FastWriter的使用

#include <iostream>             // 包含“标准输入输出头文件” ---> 用于打印JSON序列化结果
#include <string>               // 包含“字符串处理头文件” ---> 用于存储JSON序列化后的字符串
#include <jsoncpp/json/json.h>  // 包含“JsonCpp库的核心头文件” ---> 用于JSON数据的构建与序列化


int main()
{
    /*------------------------------- 第一步：创建对象 -------------------------------*/
    //1.创建JSON对象（JSON根节点，本质是一个JSON对象）
    Json::Value root; // Json::Value是JsonCpp库的核心类型，可表示JSON中的对象、数组、字符串、数字等

    /*------------------------------- 第二步：添加键值对 -------------------------------*/
    //1.向JSON对象中添加键值对：字符串类型
    root["name"] = "张三";  // 键"name"，值为字符串"张三"
    root["sex"] = "男";     // 键"sex"，值为字符串"男"

    //2.向JSON对象中添加键值对：整数类型
    root["age"] = 18;       // 键"age"，值为整数18

    /*------------------------------- 第三步：JSON序列化 -------------------------------*/
    //1.使用FastWriter（无格式压缩输出，适合网络传输，减少数据量）
    Json::FastWriter writer;   // 输出结果为一行字符串，无换行和缩进，如{"age":18,"name":"张三","sex":"男"}

    //2.将Json::Value对象序列化为字符串
    std::string s = writer.write(root);
    //3.打印序列化后的JSON字符串
    std::cout << s << std::endl;

    return 0;
}