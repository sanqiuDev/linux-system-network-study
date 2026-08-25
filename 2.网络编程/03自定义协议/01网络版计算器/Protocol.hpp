#pragma once  

#include <iostream>             // 标准输入输出 ---> 预留调试使用）
#include <string>               // 字符串处理 ---> 序列化/反序列化的载体）
#include <memory>               // 智能指针 ---> 用于Socket对象管理）
#include <jsoncpp/json/json.h>  // JsonCpp库 ---> JSON序列化/反序列化
#include <functional>           // 函数包装器 ---> 定义业务处理回调
#include "Socket.hpp"           // Socket模块头文件 ---> 提供Socket通信能力

//1.引入Socket模块命名空间，简化Socket相关类的使用
using namespace SocketModule;

//2.协议分隔符 ---> 用于分隔长度和JSON字符串（解决粘包/半包）
const std::string sep = "\r\n";

//3.定义业务处理回调函数类型 ---> 封装计算逻辑
//using func_t = std::function<Response (Request &req)>;
// 参数：Request &req - 客户端请求对象
// 返回值：Response - 服务器响应对象

//------------------------------------------------------------------------------------------------------------
// 实现一个自定义的网络版本计算器协议栈：
// 包含请求(Request)、响应(Response)、协议处理(Protocol)三层，
// 核心解决「数据序列化/反序列化」和「TCP粘包/半包」问题

//如何要做序列化和反序列化?
// 1. 我们自己写 ---> 往往不具备很好的扩展性
// 2. 使用现成的方案 ---> jsoncpp

// 自定义协议格式（解决TCP粘包/半包）：
//  1. 格式：content_len\r\njsonstring\r\n
//  2. 示例：50\r\n{"x": 10, "y" : 20, "oper" : '+'}\r\n
//  3. 说明：content_len是JSON字符串的长度，\r\n作为分隔符，确保能解析完整请求
//------------------------------------------------------------------------------------------------------------

/*-------------------------------------------------请求类-------------------------------------------------*/
// 请求类 ---> 封装客户端发送给服务器的计算请求（如"10 + 20"）
class Request
{
private:
    int _x;     // 第一个运算数
    int _y;     // 第二个运算数
    char _oper; // 运算符（+ - * / %），表示 _x _oper _y 的运算逻辑

public:
    /*-----------------------------------【构造&析构】-----------------------------------*/
    //1.“默认构造函数” ---> 创建空请求对象（用于反序列化时初始化）
    Request(){}

    //2.“带参构造函数” ---> 通过具体的计算参数创建请求对象
    Request(int x, int y, char oper)
     : _x(x),
      _y(y), 
      _oper(oper)
    { }

    //3.“析构函数” ---> 空实现（无动态分配资源）
    ~Request() {}


    /*-----------------------------------【序列化&反序列化】-----------------------------------*/
    //4.“序列化” ---> 将请求对象转换为可网络传输的字符串
    std::string Serialize()
    {
        // //1.示例序列化逻辑：用空格分隔字段，如"10 20 +"
        // return std::to_string(_x) + " " + std::to_string(_y) + " " + _oper;


        //1.用JSON格式存储数据，便于跨语言解析
        Json::Value root;
        root["x"] = _x;          // 存储第一个运算数
        root["y"] = _y;          // 存储第二个运算数
        root["oper"] = _oper;    // 存储运算符（JsonCpp自动处理char类型）

        //2.无格式JSON序列化（紧凑字符串，减少传输量）
        Json::FastWriter writer; 
        std::string s = writer.write(root);

        //3.返回序列化后的JSON字符串
        return s;
    }

    //5.“反序列化” ---> 将网络接收的字符串还原为请求对象
    bool Deserialize(std::string &in)
    {
        // //1.示例反序列化逻辑：按空格分割字符串，解析出_x、_y、_oper
        // size_t pos1 = in.find(' ');
        // size_t pos2 = in.find(' ', pos1 + 1);
        // if (pos1 == std::string::npos || pos2 == std::string::npos)
        // {
        //    return false;
        // }

        // _x = std::stoi(in.substr(0, pos1));
        // _y = std::stoi(in.substr(pos1 + 1, pos2 - pos1 - 1));
        // _oper = in[pos2 + 1];
        // return true;


        //1.准备反序列化的核心对象
        Json::Value root;
        Json::Reader reader;  // JSON解析器

        //2.解析JSON字符串到Json::Value
        bool ok = reader.parse(in, root); 
        if (ok)
        {
            _x = root["x"].asInt();      // 提取整数类型的x
            _y = root["y"].asInt();      // 提取整数类型的y
            _oper = root["oper"].asInt();// 注意：JsonCpp中char会被解析为int，需强转（实际应为asString()[0]）
        }

        //3.返回反序列化操作的结果
        return ok;
    }

    /*-----------------------------------【get方法】-----------------------------------*/
    //6.获取第一个运算数
    int X(){return _x;}    
    //7.获取第二个运算数
    int Y(){return _y;}    
    //8.获取运算符
    char Oper(){return _oper;} 
};


/*-------------------------------------------------响应类-------------------------------------------------*/
// 响应类 ---> 封装服务器返回给客户端的计算结果
class Response
{
private:
    //1.运算结果 ---> 需配合_code判断是否为有效结果
    //2.状态码 ---> 0=成功，1=除零错误，2=无效运算符，3=数值溢出等
    int _result; 
    int _code;   

public:
    /*-----------------------------------【构造&析构】-----------------------------------*/
    //1.“默认构造函数” ---> 创建空响应对象（用于反序列化时初始化）
    Response() {}

    //2.“带参构造函数” ---> 通过计算结果和状态码创建响应对象
    Response(int result, int code)
    : _result(result),
     _code(code)
    {}

    //3.“析构函数” ---> 空实现
    ~Response() {}


    /*-----------------------------------【序列化&反序列化】-----------------------------------*/
    //4.“序列化” ---> 将响应对象转换为可网络传输的字符串
    std::string Serialize()
    {
        // //1.示例序列化逻辑：用空格分隔结果和状态码，如"30 0"
        // return std::to_string(_result) + " " + std::to_string(_code);

        //1.用JSON格式存储数据，便于跨语言解析
        Json::Value root;
        root["result"] = _result; // 存储运算结果
        root["code"] = _code;     // 存储状态码

        //2.无格式JSON序列化（紧凑字符串，减少传输量）
        Json::FastWriter writer;
        //3.返回序列化后的JSON字符串
        return writer.write(root); 
    }

    //5.“反序列化：” ---> 将网络接收的字符串还原为响应对象
    bool Deserialize(std::string &in)
    {
        // //1.示例反序列化逻辑：按空格分割字符串，解析出_result和_code
        // size_t pos = in.find(' ');
        // if (pos == std::string::npos)
        //     return false;

        // _result = std::stoi(in.substr(0, pos));
        // _code = std::stoi(in.substr(pos + 1));
        // return true;


        //1.准备反序列化的核心对象
        Json::Value root;
        Json::Reader reader;  // JSON解析器

        //2.解析JSON字符串到Json::Value
        bool ok = reader.parse(in, root); 
        if (ok)
        {
            _result = root["result"].asInt(); // 提取运算结果
            _code = root["code"].asInt();     // 提取状态码
        }

        //3.返回反序列化操作的结果
        return ok;
    }

    /*-----------------------------------【set方法】-----------------------------------*/
    //6.设置运算结果
    void SetResult(int res) {_result = res;} 
    //7.设置状态码
    void SetCode(int code) {_code = code;}   

    //8.打印运算结果和状态码（客户端调用）
    void ShowResult()  // 状态码说明：0=成功，1=除零错误，2=模零错误，3=非法运算符
    {
        std::cout << "计算结果是: " << _result << "[" << _code << "]" << std::endl;
    }
};

using func_t = std::function<Response (Request &req)>;

/*-------------------------------------------------协议处理类-------------------------------------------------*/
// 协议处理类 ---> 封装完整的TCP通信协议逻辑（请求接收、解析、处理、响应发送）
class Protocol
{
private:
    // 因为我们用的是多进程模型：每个子进程独立处理一个客户端请求，
    // 所以不需要类成员存储Request/Response（避免多进程间资源竞争），
    // 而是在GetRequest函数内创建局部对象即可
    Request _req;
    Response _resp;
    func_t _func; 

public:
    /*-----------------------------------【构造&析构】-----------------------------------*/
    //1.“默认构造函数” ---> 创建空协议处理对象
    Protocol(){}

    //2.“构造函数” ---> 绑定业务处理回调函数
    Protocol(func_t func):_func(func)
    { }

    //3.“析构函数” ---> 空实现
    ~Protocol() {}

    /*-----------------------------------【编码&解码】-----------------------------------*/
    //3.“编码” ---> 给JSON字符串添加长度前缀（解决TCP粘包/半包）
    std::string Encode(const std::string &jsonstr)
    {
        //1.JSON字符串长度
        std::string len = std::to_string(jsonstr.size());

        //2.拼接长度、分隔符、JSON字符串、分隔符
        return len + sep + jsonstr + sep;  // 格式：content_len\r\njsonstring\r\n
    }

    //4.“解码” ---> 从缓冲区中提取完整的JSON报文（解决TCP粘包/半包）
    bool Decode(std::string &buffer, std::string *package)
    {
        //1.查找第一个分隔符（判断是否包含长度部分）
        ssize_t pos = buffer.find(sep);
        if (pos == std::string::npos)
        {
            return false; // 无分隔符，缓冲区数据不完整，需继续读取
        }

        //2.提取JSON字符串的长度前缀部分并将其转换为整数
        //2.1：提取
        std::string package_len_str = buffer.substr(0, pos); 
        //2.2：转换
        int package_len_int = std::stoi(package_len_str);    

        //3.根据完整报文的长度判断缓冲区是否包含完整报文
        //3.1：计算
        int target_len = package_len_str.size() + package_len_int + 2 * sep.size();
        //注意：完整报文长度 = 长度字符串长度 + 分隔符长度*2 + JSON字符串长度
        //3.2：判断
        if (buffer.size() < target_len)
        {
            return false; // 缓冲区数据不足，需继续读取
        }

        //4.提取完整的JSON字符串并更新缓冲区
        //4.1：提取
        *package = buffer.substr(pos + sep.size(), package_len_int); 
        //4.2：更新
        buffer.erase(0, target_len); // 从缓冲区中移除已处理的完整报文（处理粘包）

        //5.成功提取完整报文
        return true; 
    }
 
    /*-----------------------------------【处理请求&解析响应】-----------------------------------*/
    //5.“服务端处理客户端请求” ---> 完整的协议交互流程
    void GetRequest(std::shared_ptr<Socket> &sock, InetAddr &client)
    {
        //1.定义缓冲区 ---> 存储从内核读取的网络数据（处理半包）
        std::string buffer_queue;  

        //2.循环的进行读取数据
        while (true)
        {
            //2.1：读取到数据从Socket读取数据到缓冲区
            int n = sock->Recv(&buffer_queue);

            //情况一：读取到数据
            if(n > 0) 
            {
                // 步骤1：存储解码后的完整JSON报文
                std::string json_package; 

                // 步骤2：解码缓冲区，提取完整的JSON请求
                bool ret = Decode(buffer_queue, &json_package);
                if(!ret)
                {
                    continue; // 报文不完整，继续读取数据
                }

                // 步骤3：JSON字符串反序列化为Request对象
                Request req;
                bool ok = req.Deserialize(json_package);
                if(!ok)
                {
                    continue; // 反序列化失败，跳过该请求
                }

                // 步骤4：调用业务回调函数处理请求（计算逻辑）
                Response resp = _func(req);

                // 步骤5：响应对象序列化为JSON字符串
                std::string json_str = resp.Serialize();

                // 步骤6：编码JSON字符串（添加长度前缀）
                std::string send_str = Encode(json_str); // 生成完整协议报文

                // 步骤7：发送响应报文给客户端
                sock->Send(send_str);
            }

            //情况二：客户端关闭连接
            else if(n == 0) 
            {
                LOG(LogLevel::INFO) << "client:" << client.StringAddr() << "Quit!";
                break; // 退出循环，结束通信
            }

            //情况三：读取数据错误
            else 
            {
                LOG(LogLevel::WARNING) << "client:" << client.StringAddr() << ", recv error";
                break; // 退出循环，结束通信
            }
        }
    }


    //6.“客户端解析服务器响应” ---> 处理粘包/半包，提取完整响应
    bool GetResponse(std::shared_ptr<Socket> &client, std::string &resp_buff, Response *resp)
    {
        //1.面向字节流，需循环读取直到获取完整响应
        while (true)
        {
            //1.1：从Socket读取数据到缓冲区
            int n = client->Recv(&resp_buff);

            //情况一：读取到数据
            if (n > 0) 
            {
                //第一步：存储解码后的完整JSON报文
                std::string json_package; 

                //第二步：循环解码缓冲区，并将JSON字符串反序列化为Response对象
                while (Decode(resp_buff, &json_package))
                {
                    resp->Deserialize(json_package);
                }

                //第三步：至少解析到一个完整响应
                return true; 
            }
            
            //情况二：服务器关闭连接
            else if (n == 0) 
            {
                std::cout << "server quit " << std::endl;
                return false;
            }

            //情况三：读取数据错误
            else 
            {
                std::cout << "recv error" << std::endl;
                return false;
            }
        }
    }

    /*-----------------------------------【构建请求】-----------------------------------*/
    //7.“客户端构建请求字符串” ---> 封装参数→序列化→编码
    std::string BuildRequestString(int x, int y, char oper)
    {
        //1.构建Request对象
        Request req(x, y, oper);

        //2.序列化Request为JSON字符串
        std::string json_req = req.Serialize();

        //3.编码JSON字符串（添加长度前缀）
        return Encode(json_req);
    }
};