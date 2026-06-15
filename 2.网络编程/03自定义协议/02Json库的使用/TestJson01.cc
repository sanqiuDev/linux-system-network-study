//JsonCpp库中toStyledString的使用

#include <iostream>             // 包含“标准输入输出头文件” ---> 用于打印JSON序列化结果
#include <string>               // 包含“字符串处理头文件” ---> 用于存储JSON序列化后的字符串
#include <jsoncpp/json/json.h>  // 包含“JsonCpp库的核心头文件” ---> 用于JSON数据的构建与序列化

int main()
{
    /*------------------------------- 第一步：创建对象 -------------------------------*/
    //1.创建Json对象（JSON根节点，本质是一个JSON对象）
    Json::Value root; // Json::Value是JsonCpp库的核心类型，可表示JSON中的对象、数组、字符串、数字等

    /*------------------------------- 第二步：添加键值对 -------------------------------*/
    //1.向JSON对象中添加键值对：字符串类型
    root["name"] = "张三";  // 键"name"，值为字符串"张三"
    root["sex"] = "男";     // 键"sex"，值为字符串"男"

    //2.向JSON对象中添加键值对：整数类型
    root["age"] = 18;       // 键"age"，值为整数18


    /*------------------------------- 第三步：创建JSON嵌套对象（可选） -------------------------------*/
    //1.创建子JSON对象（用于存储嵌套数据）
    Json::Value sub;
    sub["电话"] = "110";  // 子对象的键值对：电话
    sub["籍贯"] = "未知"; // 子对象的键值对：籍贯
    
    //2.将子对象作为根对象的一个键值对（实现JSON嵌套）
    root["info"] = sub;

    /*------------------------------- 第四步：快速序列化 -------------------------------*/
    //3.调用Json::Value的toStyledString()方法快速序列化（等价于StyledWriter）
    std::string s = root.toStyledString();
    std::cout << s << std::endl;

    return 0;
}