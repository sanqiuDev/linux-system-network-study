#ifndef _UTIL_H_
#define _UTIL_H_

#include "logger.hpp"  

#include <iostream>    
#include <sstream>     
#include <fstream>     
#include <string>      
#include <memory>      
#include <vector>    
#include <cstdint>     

#include <jsoncpp/json/json.h>     
#include <mysql/mysql.h>          
#include <websocketpp/server.hpp> 
#include <websocketpp/config/asio_no_tls.hpp> 



// wsserver_t == 基于asio_no_tls配置的WebSocket服务器类
typedef websocketpp::server<websocketpp::config::asio> wsserver_t;


//1.数据库操作工具类（静态类）
class mysql_util 
{
    public:
        static MYSQL *mysql_create(const std::string &host,
            const std::string &username,
            const std::string &password,
            const std::string &dbname,
            uint16_t port = 3306) 
        {
            //1.初始化MySQL句柄
            MYSQL *mysql = mysql_init(NULL);
            if (mysql == NULL) 
            {
                ELOG("mysql init failed!");
                return NULL;
            }

            //2.建立与MySQL服务器的网络连接
            if (mysql_real_connect(mysql, 
                host.c_str(), 
                username.c_str(), 
                password.c_str(), 
                dbname.c_str(), port, NULL, 0) == NULL) 
            {
                ELOG("connect mysql server failed : %s", mysql_error(mysql));
                mysql_close(mysql);
                return NULL;
            }

            //3.设置客户端与服务器通信的字符集为utf8
            if (mysql_set_character_set(mysql, "utf8") != 0) 
            {
                ELOG("set client character failed : %s", mysql_error(mysql));
                mysql_close(mysql);
                return NULL;
            }

            //4.返回有效的MySQL连接句柄
            return mysql;
        }

        /**
         * @brief 执行指定的SQL语句
         * @param mysql 已初始化并成功连接的MySQL句柄指针
         * @param sql 要执行的SQL语句（支持SELECT/INSERT/UPDATE/DELETE等）
         * @return bool 执行成功返回true，失败返回false
         */
        static bool mysql_exec(MYSQL *mysql, const std::string &sql)
        {
            // 1.调用mysql_query执行SQL语句，c_str()转换std::string为const char*
            int ret = mysql_query(mysql, sql.c_str());
            if (ret != 0)
            {
                ELOG("%s\n", sql.c_str());
                ELOG("mysql query failed : %s\n", mysql_error(mysql));
                return false;
            }

            // 2.SQL执行成功，返回true
            return true;
        }

        /**
         * @brief 销毁MySQL连接句柄，关闭数据库连接并释放内存
         * @param mysql 要销毁的MySQL句柄指针（可传入NULL，内部做判空处理）
         * @return void 无返回值
         */
        static void mysql_destroy(MYSQL *mysql)
        {
            if (mysql != NULL)
            {
                mysql_close(mysql);
            }
            return;
        }
};



//2. JSON数据操作工具类（静态类）
class json_util
{
    public:
        static bool serialize(const Json::Value &root, std::string &str) 
        {
            //1.实例化StreamWriterBuilder工厂类对象，用于创建StreamWriter序列化对象
            Json::StreamWriterBuilder swb;

            //2.使用std::unique_ptr智能指针管理StreamWriter对象，自动释放内存（无需手动delete）
            // newStreamWriter()：通过工厂类创建StreamWriter序列化执行对象
            std::unique_ptr<Json::StreamWriter>sw(swb.newStreamWriter());

            //3.定义stringstream对象，用于在内存中存储序列化后的JSON数据（避免直接操作文件）
            std::stringstream ss;

            //4.调用write()方法执行序列化，将Json::Value数据写入stringstream
            int ret = sw->write(root, &ss);
            if (ret != 0)  
            {
                ELOG("json serialize failed!");
                return false;
            }

            //5.将stringstream中的数据转换为std::string，通过输出参数返回
            str = ss.str();
            return true;
        }

        /**
         * @brief JSON反序列化：将JSON格式字符串转换为Json::Value结构化数据
         * @param str 待反序列化的JSON格式字符串
         * @param root 输出参数，用于存储反序列化后的结构化数据
         * @return bool 反序列化成功返回true，失败返回false
         */
        static bool unserialize(const std::string &str, Json::Value &root) 
        {
            //1.实例化CharReaderBuilder工厂类对象，用于创建CharReader反序列化对象
            Json::CharReaderBuilder crb;

            //2.使用std::unique_ptr智能指针管理CharReader对象，自动释放内存
            // newCharReader()：通过工厂类创建CharReader反序列化执行对象
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());

            //3.定义错误信息字符串，用于接收反序列化过程中的详细错误信息
            std::string err;

            //4.调用parse()方法执行反序列化，解析JSON字符串并存储结果到Json::Value
            // 参数说明：字符串起始地址、字符串结束地址、结果存储对象、错误信息对象
            bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), &root, &err);
            if (ret == false) 
            {
                ELOG("json unserialize failed: %s", err.c_str());
                return false;
            }

            //5.反序列化成功，返回true
            return true;
        }
};

/**
 * @brief 字符串操作工具类（静态类）
 * 封装了常用的字符串处理功能，当前实现了字符串分割功能
 * 功能：按指定分隔符分割字符串，将结果存入vector容器
 */
class string_util
{
  public:
        /**
         * @brief 字符串分割：按指定分隔符将源字符串分割为多个子字符串
         * @param src 待分割的源字符串（如："123,456,789"）
         * @param sep 分隔符字符串（如：","，支持多字符分隔符）
         * @param res 输出参数，vector容器，用于存储分割后的子字符串集合
         * @return int 返回分割后的子字符串个数（即res的大小）
         */
        static int split(const std::string &src, const std::string &sep, std::vector<std::string> &res) 
        {
            // 示例：src = "123,234,,,,345"，sep = ","

            //1.定义变量pos存储分隔符的位置，idx遍历字符串的当前索引
            size_t pos, idx = 0; 

            //2.循环遍历源字符串，直到索引超出字符串长度
            while(idx < src.size()) 
            {
                //2.1：从idx位置开始查找分隔符sep，返回分隔符首次出现的位置
                pos = src.find(sep, idx);

                //2.2：分情况讨论
                // 情况1：未找到分隔符（pos == std::string::npos），说明剩余部分为最后一个子字符串
                if (pos == std::string::npos) 
                {
                    // 截取从idx到字符串末尾的子字符串，存入结果容器
                    res.push_back(src.substr(idx));

                    // 跳出循环，分割完成
                    break; 
                }

                // 情况2：分隔符位于当前索引位置（连续分隔符，如：",,"），直接跳过当前分隔符
                if (pos == idx) 
                {
                    // 索引向后移动分隔符长度，跳过当前分隔符
                    idx += sep.size(); 

                    // 继续下一次循环，查找下一个分隔符
                    continue; 
                }

                // 情况3：找到有效分隔符，截取从idx到pos的子字符串（不包含分隔符）
                res.push_back(src.substr(idx, pos - idx));

                //2.3：索引向后移动，跳过已处理的子字符串和分隔符
                idx = pos + sep.size();
            }

            //3.返回分割后的子字符串个数
            return res.size();
        }
};

/**
 * @brief 文件操作工具类（静态类）
 * 封装了常用的文件读取功能，当前实现了二进制方式读取整个文件内容
 * 功能：将指定文件的全部内容读取到字符串中，支持任意格式文件（文本/二进制）
 */
class file_util 
{
   public:
        /**
         * @brief 读取文件全部内容：以二进制方式读取文件，将内容存入字符串
         * @param filename 要读取的文件路径及文件名（如："./index.html"）
         * @param body 输出参数，用于存储读取到的文件内容（二进制数据）
         * @return bool 读取成功返回true，失败返回false
         */
        static bool read(const std::string &filename, std::string &body) 
        {
            //1.以二进制模式打开文件（std::ios::binary），避免文本模式下的换行符转换等问题
            // ifstream：C++输入文件流，用于从文件中读取数据
            std::ifstream ifs(filename, std::ios::binary);
            if (ifs.is_open() == false) 
            {
                ELOG("%s file open failed!!", filename.c_str());
                return false;
            }

            //2.获取文件大小
            //2.1：seekg(0, std::ios::end)将文件读取指针移动到文件末尾
            ifs.seekg(0, std::ios::end);
            //2.2：tellg()获取当前文件读取指针的位置（即文件大小，字节数）
            size_t fsize = 0;
            fsize = ifs.tellg();
            //2.3：seekg(0, std::ios::beg)将文件读取指针移回文件开头，准备读取内容
            ifs.seekg(0, std::ios::beg);

            //3.调整输出字符串的大小，预留足够空间存储文件内容（避免多次内存分配）
            body.resize(fsize);

            //4.一次性读取整个文件内容到字符串中
            ifs.read(&body[0], fsize); // &body[0]：获取字符串的首地址（body已resize，内存连续分配）
            if (ifs.good() == false) //good()：检查流状态是否正常，无读取错误/到达文件末尾等问题
            {
                ELOG("read %s file content failed!", filename.c_str());
                ifs.close(); 
                return false;
            }

            //5.读取成功，关闭文件，释放文件句柄
            ifs.close();
            return true;
        }
};

// 结束头文件保护宏
#endif