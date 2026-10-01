// 头文件保护宏：防止该头文件被重复包含，避免类/宏重定义编译错误
#ifndef __M_DB_H__
#define __M_DB_H__

#include "util.hpp"  // 引入自定义工具类头文件，包含MySQL操作、JSON处理等工具
#include <mutex>     // 引入C++标准互斥锁库 -> 用于实现线程安全，保护数据库并发访问
#include <cassert>   // 引入C++断言库 -------> 用于调试阶段的合法性校验（运行时断言失败会终止程序）

/**
 * @brief 用户表业务操作类
 * 封装了用户表（user）的所有数据库操作（插入、查询、更新等），提供面向业务的接口
 * 特性：1. 依赖mysql_util实现底层数据库操作；2. 提供互斥锁保证并发访问安全；3. 自动管理数据库连接句柄
 */
class user_table
{
   private:
          //1.MySQL操作句柄，用于与数据库建立连接并执行操作
          //2.互斥锁，保护数据库的并发访问（防止多线程同时操作数据库导致数据混乱）
          MYSQL *_mysql;          
          std::mutex _mutex;      
   public:
          /**
           * @brief 构造函数：初始化用户表对象，建立数据库连接
           * @param host 数据库服务器地址（如："127.0.0.1"）
           * @param username 数据库登录用户名（如："root"）
           * @param password 数据库登录密码
           * @param dbname 目标数据库名（如："gobang"）
           * @param port 数据库服务端口号（默认3306，MySQL默认端口）
           */
          user_table(const std::string &host,
               const std::string &username,
               const std::string &password,
               const std::string &dbname,
               uint16_t port = 3306) 
          {
               //1.调用mysql_util工具类，创建MySQL连接句柄并建立数据库连接
               _mysql = mysql_util::mysql_create(host, username, password, dbname, port);

               //2.断言校验：调试阶段确保数据库连接成功，_mysql不为NULL（发布环境可禁用assert）
               assert(_mysql != NULL);
          }

          /**
           * @brief 析构函数：销毁用户表对象，释放数据库连接资源
           * 自动调用mysql_util工具类关闭数据库连接，避免内存泄漏
           */
          ~user_table() 
          {
               //1.调用mysql_util工具类，关闭数据库连接并释放句柄内存
               mysql_util::mysql_destroy(_mysql);

               //2.将句柄置为NULL，避免悬空指针（后续访问可通过NULL判断句柄是否有效）
               _mysql = NULL;
          }

          /**
           * @brief 业务接口：用户注册，向用户表中插入新用户数据
           * @param user 输入输出参数，Json::Value对象，需包含"username"和"password"字段
           * @return bool 插入成功返回true，失败返回false
           */
          bool insert(Json::Value &user) 
          {
               //1.定义插入用户的SQL语句宏，简化字符串编写，提高可读性
               // null：对应user表的自增id字段，数据库自动赋值；后续依次为用户名、密码、初始分数、总场次、胜利场次
#define INSERT_USER "insert user values(null, '%s', '%s', 1000, 0, 0);"

               //2.入参合法性校验：检查用户名和密码字段是否存在（避免空值插入数据库）
               if (user["password"].isNull() || user["username"].isNull()) 
               {
                    DLOG("INPUT PASSWORD OR USERNAME");  
                    return false;
               }

               //3.格式化SQL语句：将用户名和密码填入SQL宏，生成完整可执行的SQL字符串
               //3.1：定义足够大的缓冲区存储SQL语句（避免缓冲区溢出）
               char sql[4096] = {0}; 
               //3.2：sprintf：将格式化数据写入字符串
               // asCString()将Json::Value字符串转为const char*，适配C语言接口
               sprintf(sql, INSERT_USER, user["username"].asCString(), user["password"].asCString());

               //4.执行SQL语句，插入用户数据
               bool ret = mysql_util::mysql_exec(_mysql, sql);
               if (ret == false) 
               {
                    DLOG("insert user info failed!!\n");  
                    return false;
               }

               //5.插入成功，返回true
               return true;
          }

          /**
           * @brief 业务接口：用户登录验证，校验用户名密码并返回用户详细信息
           * @param user 输入输出参数，输入"username"和"password"，输出用户id、分数、场次等信息
           * @return bool 登录成功（用户名密码匹配）返回true，失败返回false
           */
          bool login(Json::Value &user) 
          {
               //1.定义登录验证的SQL语句宏，根据用户名和密码查询用户核心信息
               // 查询字段：id（用户唯一标识）、score（天梯分数）、total_count（总战斗场次）、win_count（胜利场次）
#define LOGIN_USER "select id, score, total_count, win_count from user where username='%s' and password='%s';"

               //2.入参合法性校验：检查用户名和密码字段是否存在
               if (user["password"].isNull() || user["username"].isNull()) 
               {
                    DLOG("INPUT PASSWORD OR USERNAME");   
                    return false;
               }
               
               //3.格式化SQL语句：填入用户名和密码，生成完整查询SQL
               char sql[4096] = {0};
               sprintf(sql, LOGIN_USER, user["username"].asCString(), user["password"].asCString());

               //4.定义结果集指针，用于存储查询返回的数据
               MYSQL_RES *res = NULL;
               {
                    //4.1：加互斥锁：使用std::unique_lock自动加锁/解锁，保证并发查询安全
                    std::unique_lock<std::mutex> lock(_mutex); 

                    //4.2：执行SQL查询语句
                    bool ret = mysql_util::mysql_exec(_mysql, sql);
                    if (ret == false) 
                    {
                         DLOG("user login failed!!\n");  
                         return false;
                    }

                    //4.3：加载查询结果集到本地内存（仅SELECT语句有效）
                    res = mysql_store_result(_mysql); // 注：用户名和密码是唯一组合，查询结果要么为空，要么只有一条记录
                    if (res == NULL) 
                    {
                         DLOG("have no login user info!!");  
                         return false;
                    }
               }  // 此处unique_lock超出作用域，自动释放互斥锁，其他线程可访问数据库

               //5.校验查询结果集的行数，确保用户信息唯一（避免同一账号重复数据）
               int row_num = mysql_num_rows(res);
               if (row_num != 1) 
               {
                    DLOG("the user information queried is not unique!!");  
                    return false;
               }

               //6.提取查询结果集中的用户数据，存入Json::Value对象返回给调用者
               //6.1：读取单行结果（仅一行数据）
               MYSQL_ROW row = mysql_fetch_row(res);  
               //6.2：类型转换并赋值：将C语言字符串转为对应数值类型，适配Json::Value存储
               user["id"] = (Json::UInt64)std::stol(row[0]);          // 用户id（无符号64位整数）
               user["score"] = (Json::UInt64)std::stol(row[1]);       // 天梯分数（无符号64位整数）
               user["total_count"] = std::stoi(row[2]);               // 总战斗场次（整数）
               user["win_count"] = std::stoi(row[3]);                 // 胜利场次（整数）

               //7.释放结果集内存，避免内存泄漏（查询结果集需手动释放）
               mysql_free_result(res);

               //8.登录验证成功，返回true
               return true;
          }

          /**
           * @brief 业务接口：通过用户名查询用户详细信息
           * @param name 要查询的用户名
           * @param user 输出参数，存储查询到的用户详细信息
           * @return bool 查询成功（存在该用户）返回true，失败返回false
           */
          bool select_by_name(const std::string &name, Json::Value &user) 
          {
               //1.定义按用户名查询的SQL语句宏
#define USER_BY_NAME "select id, score, total_count, win_count from user where username='%s';"

               //2.格式化SQL语句：填入用户名，生成完整查询SQL
               char sql[4096] = {0};
               sprintf(sql, USER_BY_NAME, name.c_str());

               //3.定义结果集指针，存储查询返回的数据
               MYSQL_RES *res = NULL;
               {
                    //3.1：加互斥锁，保证并发查询安全
                    std::unique_lock<std::mutex> lock(_mutex);

                    //3.2：执行SQL查询语句
                    bool ret = mysql_util::mysql_exec(_mysql, sql);
                    if (ret == false) 
                    {
                         DLOG("get user by name failed!!\n");  
                         return false;
                    }

                    //3.3：加载查询结果集到本地内存
                    res = mysql_store_result(_mysql);
                    if (res == NULL) 
                    {
                         DLOG("have no user info!!");  
                         return false;
                    }
               }  

               //4.校验查询结果集行数，确保用户信息唯一
               int row_num = mysql_num_rows(res);
               if (row_num != 1) 
               {
                    DLOG("the user information queried is not unique!!");  // 打印调试日志，提示用户信息不唯一
                    return false;
               }

               //5.提取用户数据，存入Json::Value对象
               MYSQL_ROW row = mysql_fetch_row(res);
               user["username"] = name;                               // 用户名
               user["id"] = (Json::UInt64)std::stol(row[0]);          // 用户id
               user["score"] = (Json::UInt64)std::stol(row[1]);       // 天梯分数
               user["total_count"] = std::stoi(row[2]);               // 总战斗场次
               user["win_count"] = std::stoi(row[3]);                 // 胜利场次

               //6.释放结果集内存
               mysql_free_result(res);

               //7.查询成功，返回true
               return true;
          }

          /**
           * @brief 业务接口：通过用户id查询用户详细信息
           * @param id 要查询的用户唯一标识id
           * @param user 输出参数，存储查询到的用户详细信息
           * @return bool 查询成功（存在该用户）返回true，失败返回false
           */
          bool select_by_id(uint64_t id, Json::Value &user) 
          {
               //1.定义按用户id查询的SQL语句宏
// #define USER_BY_ID "select username, score, total_count, win_count from user where id=%d;"
#define USER_BY_ID "select username, score, total_count, win_count from user where id=%lu;"

               //2.格式化SQL语句：填入用户id，生成完整查询SQL
               char sql[4096] = {0};
               sprintf(sql, USER_BY_ID, id);

               //3.定义结果集指针，存储查询返回的数据 
               MYSQL_RES *res = NULL;
               {
                    //3.1：加互斥锁，保证并发查询安全
                    std::unique_lock<std::mutex> lock(_mutex);

                    //3.2：执行SQL查询语句
                    bool ret = mysql_util::mysql_exec(_mysql, sql);
                    if (ret == false) 
                    {
                         DLOG("get user by id failed!!\n");  
                         return false;
                    }

                    //3.3：加载查询结果集到本地内存
                    res = mysql_store_result(_mysql);
                    if (res == NULL) 
                    {
                         DLOG("have no user info!!");  
                         return false;
                    }
               }  

               //4.校验查询结果集行数，确保用户信息唯一
               int row_num = mysql_num_rows(res);
               if (row_num != 1) 
               {
                    DLOG("the user information queried is not unique!!");  // 打印调试日志，提示用户信息不唯一
                    return false;
               }

               //5.提取用户数据，存入Json::Value对象
               MYSQL_ROW row = mysql_fetch_row(res);
               user["id"] = (Json::UInt64)id;                         // 用户id
               user["username"] = row[0];                             // 用户名
               user["score"] = (Json::UInt64)std::stol(row[1]);       // 天梯分数
               user["total_count"] = std::stoi(row[2]);               // 总战斗场次
               user["win_count"] = std::stoi(row[3]);                 // 胜利场次

               //6.释放结果集内存
               mysql_free_result(res);

               //7.查询成功，返回true
               return true;
          }


          /**
           * @brief 业务接口：用户对战胜利，更新用户数据（加分、增加场次和胜利场次）
           * @param id 胜利用户的唯一标识id
           * @return bool 更新成功返回true，失败返回false
           */
          bool win(uint64_t id) 
          {
               //1.定义用户胜利的更新SQL语句宏
               // 更新逻辑：天梯分数+30，总战斗场次+1，胜利场次+1
// #define USER_WIN "update user set score=score+30, total_count=total_count+1, win_count=win_count+1 where id=%d;"
#define USER_WIN "update user set score=score+30, total_count=total_count+1, win_count=win_count+1 where id=%lu;"

               //2.格式化SQL语句：填入用户id，生成完整更新SQL
               char sql[4096] = {0};
               sprintf(sql, USER_WIN, id);

               //3.执行SQL更新语句
               bool ret = mysql_util::mysql_exec(_mysql, sql);
               if (ret == false) 
               {
                    DLOG("update win user info failed!!\n");  
                    return false;
               }

               //4.更新成功，返回true
               return true;
          }

          /**
           * @brief 业务接口：用户对战失败，更新用户数据（扣分、增加场次）
           * @param id 失败用户的唯一标识id
           * @return bool 更新成功返回true，失败返回false
           */
          bool lose(uint64_t id) 
          {
               //1.定义用户失败的更新SQL语句宏
               // 更新逻辑：天梯分数-30，总战斗场次+1，胜利场次不变
// #define USER_LOSE "update user set score=score-30, total_count=total_count+1 where id=%d;"
#define USER_LOSE "update user set score=score-30, total_count=total_count+1 where id=%lu;"

               //2.格式化SQL语句：填入用户id，生成完整更新SQL
               char sql[4096] = {0};
               sprintf(sql, USER_LOSE, id);

               //3.执行SQL更新语句
               bool ret = mysql_util::mysql_exec(_mysql, sql);
               if (ret == false) 
               {
                    DLOG("update lose user info failed!!\n");  
                    return false;
               }

               //4.更新成功，返回true
               return true;
          }
};

// 结束头文件保护宏
#endif