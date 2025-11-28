#include <iostream>             // 包含标准输入输出头文件 ---> 用于打印JSON序列化结果
#include <string>               // 包含字符串处理头文件 ---> 用于存储JSON序列化后的字符串
#include <jsoncpp/json/json.h>  // 包含JsonCpp库的核心头文件 ---> 用于JSON数据的构建与序列化

// 示例：若需要序列化自定义类（如person），可在此定义类并实现序列化逻辑
// class person
// {
// public:
//     std::string name;
//     int age;
//     // 序列化方法：将类对象转换为Json::Value
//     Json::Value Serialize() {
//         Json::Value root;
//         root["name"] = name;
//         root["age"] = age;
//         return root;
//     }
// };

int main()
{
    //1.创建Json::Value对象（JSON根节点，本质是一个JSON对象）
    Json::Value root; // Json::Value是JsonCpp库的核心类型，可表示JSON中的对象、数组、字符串、数字等

    //2.向JSON对象中添加键值对：字符串类型
    root["name"] = "张三";  // 键"name"，值为字符串"张三"
    root["sex"] = "男";     // 键"sex"，值为字符串"男"

    //3.向JSON对象中添加键值对：整数类型
    root["age"] = 18;       // 键"age"，值为整数18

    // ========== JSON序列化方式选择 ==========
    // 方式1：使用FastWriter（无格式压缩输出，适合网络传输，减少数据量）
    // Json::FastWriter writer; // 输出结果为一行字符串，无换行和缩进，如{"age":18,"name":"张三","sex":"男"}
    // 方式2：使用StyledWriter（带格式美化输出，可读性强，适合调试）
    Json::StyledWriter writer; // 输出结果带换行和缩进，便于人类阅读

    // 将Json::Value对象序列化为字符串
    std::string s = writer.write(root);

    // 打印序列化后的JSON字符串
    std::cout << s << std::endl;

    // ========== 扩展：JSON嵌套对象 ==========
    // // 创建子JSON对象（用于存储嵌套数据）
    // Json::Value sub;
    // sub["tel"] = "12345";    // 子对象的键值对：电话
    // sub["籍贯"] = "XXX";     // 子对象的键值对：籍贯
    // // 将子对象作为根对象的一个键值对（实现JSON嵌套）
    // root["info"] = sub;
    // // 也可直接调用Json::Value的toStyledString()方法快速序列化（等价于StyledWriter）
    // std::string s = root.toStyledString();
    // std::cout << s << std::endl;

    return 0;
}