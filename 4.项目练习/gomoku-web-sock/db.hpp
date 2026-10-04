//先进行头文件的保护
#ifndef DB_HPP
#define DB_HPP

#include "util.hpp"

#include <cassert>

//首先我们先来创建一个用户表的类
class user_table
{
    private:
    //先要去想我们需要什么属性
    //1.MySQL数据库操作句柄 ---> 因为对数据表的操作都要依靠它
    //2.一把互斥锁
    MYSQL*_mysql;
    std::mutex _mutex;

    public:
    //1.首先来实现一下构造函数
    user_table(
        const std::string &host,
        const std::string &username,
        const std::string &password,
        const std::string &dbname,
        uint16_t port=3306
    )
    {
        //用户表的构造函数主要是对属性MySQL数据库的操作句柄进行初始化
        mysql_util::mysql_create(host,username,password,dbname,port);   

        //这里可以添加一个断言
        assert(_mysql!=NULL); //注意这里要记得添加头文件<cassert>
        /*
            assert = 强行检查「必须成立的条件」
            如果条件不成立 → 程序直接崩溃并打印错误位置
            如果条件成立 → 啥事不做，继续运行



            执行完 mysql_create 之后，_mysql 到底发生了什么？本质是什么？
            _mysql 从【NULL 空指针】变成了【一个有效的、真实的数据库连接指针】！
                    _mysql = 指向数据库连接的句柄（handler）
                    你可以把它理解成：_mysql = 数据库的 “电话线 / 插座”
             mysql_create 内部一定做了这 3 件事：
                1. 调用 mysql_init (),创建一个 MYSQL 对象，分配内存
                2. 调用 mysql_real_connect (),真正和数据库建立 TCP 连接
                3. 把返回的指针赋值给 全局 / 成员变量 _mysql：_mysql = mysql_real_connect(...);
        */
    }

    //2.接着我们来实现这个析构函数
    ~user_table()
    {
        //我们主要析构的东西也就是MySQL数据库的操作句柄
        mysql_util::mysql_destroy(_mysql);

        //最后将操作数据库的句柄指针置空即可
        _mysql=NULL;
    }

    // 3.实现：用户完成注册将用户信息添加到数据库中的操作
    bool insert(const Json::Value &user)
    /*
       首先明白这里为什么传入的是Json::Value 类型的数据?
       因为前端传给后端的数据，就是 JSON 格式！后端接收、解析、取出字段，用的就是 Json::Value 对象！

       接下来我们再来解释Json::Value是个是什么东西？
        你可以把它理解成：一个能装下任何 JSON 数据的超级容器，能装：
            字符串 数字 布尔 嵌套对象 数组
        前端传过来什么，它就能装什么。

        JSON = 快递包裹
        Json::Value = 快递收纳盒
      前端把包裹发给你，你必须用 ** 收纳盒（Json::Value）** 来收
      然后才能打开拿东西（username、password）
    */
    {

        // 1.定义插入数据库的SQL语句宏
#define INSERT_USER "insert user values(null, '%s', '%s', 1000, 0, 0);" // 注意添加双引号
        /*
           insert：SQL 关键字，意思是 插入数据
           user：你数据库里的表名（用户表）
           合起来：往 user 表中插入数据

           括号里的内容，就是要插入的每一列的值！
           你的表结构一定是 6 列，顺序如下：
               id 用户 ID（主键，自增）： id 是自增主键，不需要我们手动填，填 null → 数据库自动生成编号
               username 用户名：         %s = 占位符，后面会被替换成真实用户名（如 "zhangsan"）
               password 密码：           同样是占位符，会被替换成真实密码（如 "123456"）
               score 分数（默认 1000）：  新用户默认 1000 分
               total 总场次：            新用户默认 1000 分
               win 胜利场次：            win 胜利场次
        */

        // 2.检查用户名和密码是否存在
        if (user["username"].isNull() || user["password"].isNull())
        /*
           我们拆成三段，你瞬间懂：
                ① user["username"]：这是一个 Json::Value 对象代表 JSON 里的 username 字段
                ② .isNull()：这是 Json::Value 的成员函数
                作用：判断这个字段是否为空、是否不存在！
        */
        {
            DLOG("用户名或者密码为空");
            return false;
        }


        //3.格式化SQL语句
        /*
            sprintf(sql, INSERT_USER, 用户名, 密码);
            它就是个 “填空机器”！
            格式：sprintf(往哪放, 模板, 填第一个坑, 填第二个坑);


            asCString() 是什么？
            Json::Value 是 C++ 的对象，数据库接口只认识 C 语言的字符串 const char*
            asCString() = 转换成 C 语言字符串
        */
        char sql[4096]={0};
        sprintf(sql,INSERT_USER,user["username"].asCString(),user["password"].asCString());


        //4.执行sql语句完成用户数据插入的操作
        bool ret = mysql_util::mysql_exec(_mysql,sql);

        //5.判断插入sql语句是否执行成功
        if(ret!=0)
        {
            DLOG("sql语句执行失败");
            return false;
        }
        return true;
    }


//4.实现：“验证用户名和密码并返回用户详细信息”的操作
bool login(Json::Value &user) //注意：这里不要使用const，因为后面返回用户详细信息要将数据保存到Json::Vlaue对象
{
    //1.定义登录验证的SQL语句宏
    #define LOGIN_USER "select id, score, total_count, win_count from user where username='%s' and password='%s';"

    //2.检查用户名和密码是否存在
    if(user["ussername"].isNull()||user["password"].isNull())
    {
        DLOG("用户名或密码不存在");

        return false;
    }

    //3.将用户名和密码填入SQL宏
    char sql[4096]={0};
    sprintf(sql,LOGIN_USER,user["username"].asCString(),user["password"].asCString());

    //4.定义结果集指针用于存储查询返回的数据
    /*
        MYSQL_RES*：这是 MySQL 库的类型
        意思：存放查询结果的 “数据集”
        你可以理解成：一个表格容器，存 select 查回来的数据
    */
    MYSQL_RES *res =NULL;

    //5.加锁执行数据库的访问和加载查询结果到本地的内存中
    /*
        为什么现在查询数据库的时候加锁了,
        之前向数据库中插入用户信息这种修改数据库的操作你不进行加锁，
        反而是这种不修改数据库的操作你枷锁了？
            
            【查询需要立刻拿结果 → 必须加锁】
            【插入只是执行一下，不需要立刻拿结果 → 暂时没加也能跑】

        真正原因只有 3 个，我给你讲得明明白白：
            1. 插入（insert）只是 “发指令”
                它做的事情：只发送 SQL → 让数据库自己执行执行完就结束了，不需要从数据库拿结果回来！
                所以即使多线程同时插入，MySQL 内部自己会排队，不会把 C++ 程序搞崩。
            2. 查询（login）必须 “发指令 + 拿结果”
                查询必须做两步：发 SQL + 立刻获取结果集（mysql_store_result）
                这两步必须连续、不能被打断！如果不加锁：
                    线程 A 发查询
                    线程 B 突然插进来发另一个查询
                    线程 A 去拿结果 → 拿到线程 B 的结果
                    程序直接错乱、崩溃！
    */
    {
        //5.1：加互斥锁
        std::unique_lock<std::mutex> lock(_mutex);  //智能锁

        //5.2：执行SQL的查询语句
        bool ret=mysql_util::mysql_exec(_mysql,sql);
        if(ret==false)
        {
            DLOG("SQL语句执行失败");
            return false;
        }

        //5.3：将查询结果保存结果集中
        /*
            MYSQL_RES* res = mysql_store_result(_mysql);
            作用：把查到的所有数据 → 全部拉到本地内存 → 放进 res 结果集
        */
        res= mysql_store_result(_mysql);  // 用户名和密码是唯一组合，查询结果要么为空，要么只有一条记录
        if(res==NULL)
        {
            DLOG("没有查询处结果");
            return false;
        }
    }


    //6.校验结果集的行数，确保用户信息唯一
    int row_num = mysql_num_rows(res);
    if(row_num!=1)
    {
        DLOG("用户数据不唯一")
        return false;
    }

    //7.提取查询结果集合中数据并将其保存到Json::Vlaue对象中返回给调用者
    /*
        MYSQL_ROW 就是一个「字符串数组」，用来存数据库里的一行数据！
        MYSQL_ROW = 指向一堆字符串的指针数组
        
        char* 行数据[] = 
        {
            "1",          // id
            "1000",       // score
            "0",          // total_count
            "0"           // win_count
        };

        MYSQL_ROW 这个字符串数组里，元素的顺序，到底和数据库表是怎么对应的？
        MYSQL_ROW 数组的顺序 = 你 SELECT 查询时，列的顺序！
        你登录时执行的 SQL 一定是：
        select id, score, total_count, win_count from user where username='xxx' and password='xxx'
            row[0]  → 第1列：id
            row[1]  → 第2列：score
            row[2]  → 第3列：total_count
            row[3]  → 第4列：win_count
    */
    MYSQL_ROW row=mysql_fetch_row(res); //mysql_fetch_row：取一行数据
    user["id"]=(Json::UInt64) std::stol(row[0]);  // Json::UInt64 = JsonCpp 库自己定义的 → 无符号 64 位整型就是超大号的正整数！
    user["socre"]=(Json::UInt64) std::stol(row[1]);
    user["total_count"]=std::stoi(row[2]);
    user["win_count"]=std::stoi(row[3]);

    //8.释放结果集
    mysql_free_result(res);

    //9.登录验证成功返回true
    return true;
}


//实现：“通过用户名查询用户详细信息”的操作
bool select_by_name(const std::string &name,Json::Value &user) 
//经过前面学习这个函数的参数就不难猜出来了：1）用户名 2）存储JSON串的Json::Value对象
{
    //1.定义用用户名查询SQL语句宏
    #define USER_BY_NAME "select id, score, total_count, win_count from user where username='%s';"

    //注意这里的细节就是：使用这个函数的时候用户已经完成了登录，所以我们不在需要什么验证“用户名以及密码”都必须存在的校验了
    //并且你也能看到这里我们是依据用户名查询用户的详细信息的

    //2.填入用户名生成完整的SQL语句
    char sql[4096] ={0};
    // sprintf(sql,USER_BY_NAME,user["username"].asCString());
    //注意：上面的这种写法是我们获得了前端返回的Json::Value对象填充SQL语句时这么写的
    //现在我们：1）本身就有用户名称name 2）其实现在我们是没有Json::Value信息的
    sprintf(sql,USER_BY_NAME,name.c_str());

    //3.定义获取用户详细信息的数据集合
    MYSQL_RES *res=NULL;

    //4.加锁执行数据库的访问和查询结果保存到结果集中
    {
        //4.1：定义互斥锁并用智能锁进行管理
        std::unique_lock<std::mutex> lock (_mutex);

        //4.2：执行数据库的访问操作
        bool ret = mysql_util::mysql_exec(_mysql,sql);
        if(ret==false)
        {
            ELOG("SQL语句执行失败");
            return false;
        }

        //4.3：将查询到的结果保存到结果集中
        res = mysql_store_result(_mysql);
        if(res==NULL)
        {
            ELOG("用户数据保存失败");
            return false;
        }
    }

    //5.校验数据的唯一性，就是看看结果集中是不是只有一个用户信息
    int rows_num = mysql_num_rows(res);
    if(rows_num!=1)
    {
        ELOG("用户数据不唯一");
        return false;
    }

    //6.将结果集中数据填到Json::Value对象中
    MYSQL_ROW row = mysql_fetch_row(res);
    user["username"]=name;
    
    user["id"]=(Json::UInt64)std::stol(row[0]);
    user["score"]=(Json::UInt64)std::stol(row[1]);
    user["total_count"]=std::stoi(row[2]);
    user["win_count"]=std::stoi(row[3]);

    //7.释放查询结果集
    mysql_free_result(res);
    return true;
}

//实现：“通过用户id查询用户详细信息”的操作
bool select_by_id(const uint64_t id ,Json::Value &user)
{
    //1.定义按用户id查询的SQL语句宏
#define USER_BY_ID "select username, score, total_count, win_count from user where id=%lu;"

        //2.格式化SQL语句：填入用户id，生成完整查询SQL
        char sql[4096] = { 0 };
        sprintf(sql, USER_BY_ID, id);

        //3.定义结果集指针，存储查询返回的数据 
        MYSQL_RES* res = NULL;
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


//7.实现：“用户对战胜利更新用户数据”的操作
bool win(const uint64_t id) //注意这里只需要知道用户id即可修改该用户的在MySQL中存放的数据
{
    //1.定义用户胜利时数据更新SQL语句宏
    #define USER_WIN "update user set score=score+30, total_count=total_count+1, win_count=win_count+1 where id=%lu;"

    //2.将用户id填入SQL语句形成完成查询语句
    char sql[4096]={0};
    sprintf(sql,USER_WIN,id);

    //3.执行SQL语句
    bool ret = mysql_util::mysql_exec(_mysql,sql);

    //4.
    if(ret==false)
    {
        ELOG("SQL语句执行失败");
        return false;
    }

    return true;
}

//8.实现：“用户对战失败更新用户数据”的操作
bool lose(const uint64_t id)
{
    //1.定义用户失败的更新SQL语句宏
#define USER_LOSE "update user set score=score-30, total_count=total_count+1 where id=%lu;"

        //2.格式化SQL语句：填入用户id，生成完整更新SQL
        char sql[4096] = { 0 };
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





#endif

