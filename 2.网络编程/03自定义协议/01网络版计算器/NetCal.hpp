#pragma once  

#include <iostream>      
#include "Protocol.hpp"  // 引入协议层头文件（包含Request/Response类定义）

// 计算器业务逻辑类 ---> 接收Request请求，执行计算，返回Response响应
class Cal
{
public:
    /*Execute：根据Request中的运算参数和运算符，返回计算结果Response
    *      1. 参数：req - 客户端请求对象（包含x、y、oper）
    *      2. 返回值：resp - 计算结果响应对象（包含result、code）
    */
    Response Execute(Request &req)
    {
        //1.初始化响应对象：result=0，code=0（code=0表示运算成功）
        Response resp(0, 0); 

        //2.根据请求中的运算符分支处理
        switch (req.Oper())
        {
        case '+':  // 加法运算
            resp.SetResult(req.X() + req.Y());  
            break;
        
        case '-':  // 减法运算
            resp.SetResult(req.X() - req.Y());  
            break;
        
        case '*':  // 乘法运算
            resp.SetResult(req.X() * req.Y());  
            break;
        
        case '/':  // 除法运算（需处理除零错误）
        {
            //情况一：除数为0，运算异常
            if (req.Y() == 0)  
            {
                resp.SetCode(1);  // 设置状态码1：表示除零错误
            }
            //情况二：除数合法，执行除法
            else  
            {
                resp.SetResult(req.X() / req.Y());  
            }
        }
        break;
        
        case '%':  // 取模运算（需处理模零错误）
        {
            //情况一：模数为0，运算异常
            if (req.Y() == 0) 
            {
                resp.SetCode(2);  // 设置状态码2：表示模零错误
            }
            //情况二：模数合法，执行取模
            else 
            {
                resp.SetResult(req.X() % req.Y()); 
            }
        }
        break;

        default:  // 无效运算符
            resp.SetCode(3);  // 设置状态码3：表示非法操作符错误
            break;
        }

        //3.返回计算响应（包含结果或错误码）
        return resp;  
    }
};