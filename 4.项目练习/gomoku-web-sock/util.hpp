//1.上来也是先头文件保护 + 头文件吧
#ifndef UTIL_HPP
#define UTIL_HPP

#include "logger.hpp"

#include <string>
#include <mysql/mysql.h>
#include <jsoncpp/json/json.h>    
#include <memory> 
#include <sstream>  
// #include <vector>
#include <fstream>   


#include <websocketpp/server.hpp> 
#include <websocketpp/config/asio_no_tls.hpp> 


// wsserver_t == 基于asio_no_tls配置的WebSocket服务器类
typedef websocketpp::server<websocketpp::config::asio> wsserver_t;
/*
   我给你翻译成人话：
        websocketpp：一个 WebSocket 网络库
        server：服务器
        asio：底层用的高性能网络框架
    合起来：基于 ASIO 异步模型的 WebSocket 服务器类型
*/




//先来完成数据库的工具类
class mysql_util
{
    public:
    //1.实现第一个功能就是：“数据库的创建”
    //一些人可能会好奇这个为什么会写成静态函数，这是因为：
    /* 
            1. 不需要创建对象，就能直接调用！
                普通函数：必须先 MySQLUtil obj; 再调用
                静态函数：直接 MySQLUtil::mysql_create(...)
                创建 MySQL 连接，根本不需要对象！所以用 static 最合理。
            
            2. 功能纯粹：只负责 “创建连接”
                它不访问类里的任何成员变量，只做一件事：
                连接数据库 → 返回连接指针这种工具函数，必须用 static

            3. 全局通用，到处都能用
                只要包含头文件，任何地方直接调用：MYSQL *conn = MySQLUtil::mysql_create(...);
    */
    static MYSQL *mysql_create(  //使用MYSQL，要引入头文件#include <mysql/mysql.h>      
        const std::string &host,
        const std::string &username,
        const std::string &password,
        const std::string &dbname,
        uint16_t port =3306
    )
    /*传参为什么传这5个参数
        static MYSQL *mysql_create(
            const std::string &host,      // 1. 数据库地址 ---> 数据库在那台机器？
            const std::string &username,  // 2. 用户名 ---> 登录用户名是什么？
            const std::string &password,  // 3. 密码 ---> 登录密码是什么？
            const std::string &dbname,    // 4. 要连接的库名 ---> 要连接那个数据库？
            uint16_t port = 3306          // 5. 端口（默认3306）---> 端口号是什么？
        )


      MYSQL * = 指向 MySQL 连接的 “指针”
      你可以把它理解成：👉 你和数据库之间的 “电话线 / 连接通道”
      MYSQL是 MySQL 库给你定义好的一个结构体里面藏了：连接状态、套接字、错误信息…你不需要知道细节，会用就行！
    */

    
    {
        //1. 首先进来第一步是：初始化文件句柄(mysql_init)
        MYSQL* mysql = mysql_init(NULL);
        if(mysql==NULL)
        {
            ELOG("数据库初始化失败");
            return NULL;
        }

        //2. 然后建立于mysql服务器的网络连接(mysql_real_connect，辅助：mysql_error,mysql_close)
        if(mysql_real_connect(mysql,host.c_str(),username.c_str(),password.c_str(),
        dbname.c_str(),port,NULL,0)==NULL) //后面这两个参数NULL和0不用管
        {
            ELOG("连接mysql服务器失败:%s",mysql_error(mysql));
            mysql_close(mysql);
            return NULL;
        }
        

        //3.接着就是设置客户端的服务端通信的字符集为utf8（mysql_set_character_set）
        if(mysql_set_character_set(mysql,"utf8")!=0)
        {
            ELOG("设置字符集失败：%s",mysql_error(mysql));
            mysql_close(mysql);
            return NULL;
        }

        //4. 返回有效连接句柄
         return mysql;
    }

    
    //2. 接着我们来完成第二个mysql的操作："数据库命令的执行" (mysql_query)
    static bool mysql_exec(MYSQL*mysql,const std::string &sql) //注意返回的结果时执行是否成功
    {
        //1.执行命令
        int ret=mysql_query(mysql,sql.c_str()); //这里也要注意下：string  -> C风格的字符串  ：c_str，

        //2.执行失败
        if(ret!=0) // 和设置字符集一样如果返回的结果不是0的话就说明失败了
        {
            ELOG("%s",sql);
            ELOG("mysql执行失败：%s",mysql_error(mysql));
            return false;
        }

        //3.执行成功
        return true;
         
    }


    //3.最后我们来完成最后一个操作：“销毁MYSQL的连接句柄” (mysql_close)
    static void mysql_destroy(MYSQL*mysql)
    {
        if (mysql != NULL) 
        {
            mysql_close(mysql);
        }
        return ;
    }
    
};



//接下来我们来写json的工具类
class json_util
{
    public:
    //1.先来实现关于json的第一个工具函数：序列化
    static bool serialize(const Json::Value &root,std::string &str) //JSon::Value要包含头文件#include <jsoncpp/json/json.h>    
    /*
        erialize = 把内存里的 JSON 结构（Json::Value）转换成字符串（std::string）这个过程叫：JSON 序列化
    */
   {
     //1.创建出一个专门用来写JSON的工厂,实例化工厂类StreamWriterBuilder的对象
     Json::StreamWriterBuilder swb;

     //2.用智能指针管理这个对象
     std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter()); //这里如果想要使用智能指针的是话需要包含#include <memory> 

     /*Json::StreamWriterBuilder：它是 JsonCpp 库提供的 Builder（建造者 / 工厂）
     作用：专门用来生产 JSON 写入器（StreamWriter）你可以理解：swb = 生产 “序列化笔” 的机器
     为什么要它？
        因为 JsonCpp 不让你直接 new StreamWriter()必须通过 Builder 来创建！这是官方设计规范 → 安全、统一、可配置
     
     swb.newStreamWriter()：从工厂里造出一支真正的 “序列化笔”！ Json::StreamWriterBuilder ---> Json::StreamWriter 

       Json::StreamWriterBuilder    是一个**类型**（造笔机器的类型）
             它就是一个类类型，用来创建对象 swb
       Json::StreamWriter           也是一个**类型**（笔的类型）
             智能指针 <> 里面写的，永远是：它要管理的 “对象类型”！
             看这里：swb.newStreamWriter()这个函数返回的是：Json::StreamWriter*
             它是指向 Json::StreamWriter 类型的指针！
     */

     //3.进行序列化，可以直接是read方法进行序列化
     std::stringstream ss;   //要想要使用stringstream 的话需要包含头文件#include <sstream>  
     /*
         char buf[32] = 固定大小的小黑板大小写死 32，不能变，装不下就溢出！
         std::stringstream = 无限大的智能黑板想装多少装多少，自动扩容，超级方便！
        也就是说stringstream 是动态缓冲区
        因为：JSON 长度不固定！可能 100 字节可能 10000 字节你根本不知道要开多大数组！所以必须用：stringstream 自动扩容，绝对安全！
     */
    int ret = sw->write(root,&ss);
    if(ret!=0)
    {
        ELOG("序列化失败");
        return false;
    }


    //4.将streamstring中的数据转化为字符串
    str=ss.str();
    return true;
   }


   //接下来继续json的第二个工具函数：“反序列化”
   static bool unserialize(const std::string &str,Json::Value &root)
   {
    //1.实例化CharReaderBuider工厂类对象
    Json::CharReaderBuilder crb;
    //2.创建CharReader对象并使用智能指针进行管理
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());

    //3.执行反序列化操作(parse)  parse = 把字符串 → 变回 Json::Value 结构化对象
    std::string err; //专门用来存储错误信息的字符串
    bool ret = cr->parse(str.c_str(),str.c_str()+str.size(),&root,&err);
    /*
        第 1 个参数：str.c_str()：字符串起始地址
        第 2 个参数：str.c_str() + str.size()：字符串结束地址
        第 3 个参数：&root；输出参数：解析完的 JSON 放在这里
        第 4 个参数：&err：错误信息输出


        把从 str 开头到 str 结尾的一串字符串
        解析成 JSON 结构
        放进 root 里面
        如果失败了就把错误信息放进 err
    */
     //4.判断反序列化成功还是失败
     if(ret==false)
     {
        ELOG("Json反序列化失败");
        return false;
     }

     return true;
   }

};

// 接着我们实现字符串的工具类
class string_util
{
public:
    // 先来实现一下字符串的操作：“字符串分割”
    static int split(const std::string src,const std::string sep,std::vector<std::string> &res) //添加头文件<vector>
    /*参数怎么填写，主要就是：
        1. 首先我们需要的是要分割的字符串：const和&都可以搞上
        2. 接着是分隔符：const和&都可以搞上
        3. 最后是用于存储分割后的子字符串集合：&搞上
    */
   {
       //1.先定义变量：1）存储分隔符的位置pos 2）vector索引的位置
       size_t pos,index=0;
       /*
           int = 有符号整数，可能是负数，不适合表示字符串长度 / 位置
           size_t = 无符号整数类型，专门用来表示 “长度、大小、下标、位置”
                32 位系统 → 底层是 unsigned int
                64 位系统 → 底层是 unsigned long long
           库统一约定STL string、vector、容器、sizeof、find()、size()返回值全是 size_t，统一标准
       */    
       
       while(index<src.size())
       {
          //size_t 查找结果 = 字符串.find( 要找的内容, 从哪个位置开始找 );
          //从字符串 src 的第 idx 位置开始，往后查找 sep 这个分隔符第一次出现的位置，并返回下标。
          //1.首先我们先找到分隔符第一次出现位置的下表是什么
          pos = src.find(sep,index);

          //情况一：没有找到分隔符 ---> 原始字符串就是一个子串
          if(pos==std::string::npos)
          {
            res.push_back(src.substr(index));
            /*
                substr = 从字符串里 “截取一段子串”
                字符串.substr(起始位置, 截取长度); 注：如果只有第一个参数的话就是说，默认截取长度是整个字符串
                返回值：截出来的新字符串
            */
            break;
          }
          
          //情况二：分隔符就在当前索引的位置 ---> 跳过当前分隔符
          if(pos==index)
          {
            index+=sep.size(); 
            continue;
          }

          //情况三：分隔符出现了原字符串的中间位置 ---> 将字符串分割
          res.push_back(src.substr(index,pos-index));
          index=pos+sep.size();  //记住如果想要跳过分隔符的话，一定是+sep.size()
       }

       return res.size(); //返回分割出字串的数量
   }
};


//最后我们实现文件的操作类 
class file_util
{
    public:
    //我们这次来实现的是文件的读取操作
    static bool read(const std::string &filename,std::string &body)
    /*
      read 函数：传入一个文件名 → 读取这个文件的所有二进制内容 → 把内容放到字符串 body 里返回出去
    */
   {
      //1. 首先以二进制的方式打开文件
      /*
         ifstream = input file stream（文件输入流）
         它就是 C++ 专门用来 “读文件” 的工具 / 对象
         括号里的两个参数：(filename, std::ios::binary)
                 第一个参数：filename：你要打开哪个文件？
                 第二个参数：std::ios::binary：以二进制模式打开文件！
                    文本模式（默认）：会自动把 \n 换行符转来转去，会破坏图片、音频、视频
                    二进制模式：原汁原味读取文件，不修改任何数据


         int = 用来存整数的类型
         string = 用来存字符串的类型
         ifstream = 用来「读文件」的类型
      */
       std::ifstream ifs(filename,std::ios::binary); //注意使用std::ifstream 这个类型需要添加头文件#include <fstream>  
       if(ifs.is_open()==false)
       {
         ELOG("%s,文件打开失败",filename.c_str);  //注意只能是C风格的字符串才能使用这个%s
         return false;
       }



       //2. 调整输出字符串的大小
       //2.1：将文件读取指针移动到文件的末尾
       ifs.seekg(0,std::ios::end); 
       /*
         ifs.seekg(偏移量, 参考位置); 偏移量是从基准位置 再往前 / 往后 挪多少字节
            ios::beg 文件开头
            ios::cur 当前指针所在位置
            ios::end 文件末尾
         支持三种跳转方式，而不是只能 “从开头算绝对位置”
           方式 1：从开头往后跳（等价绝对位置）:seekg(100, ios::beg);
           方式 2：从当前位置相对跳转（最常用、最省事）:
                  seekg(20, ios::cur);   // 当前位置往后走20
                  seekg(-5, ios::cur);   // 当前位置往前走5
           方式 3：从文件末尾往前倒着跳： seekg(-10, ios::end);


        tellg () ：查询当前「文件读指针」在第几字节的位置
        tellg () 返回的是：当前文件读指针 距离文件开头的字节偏移量
       */
       //2.2：获取文件读写指针的位置
       size_t fsize=0;
       fsize=ifs.tellg();
       //2.3：将文件读写指针回到文件的开头
       ifs.seekg(0,std::ios::beg);
       //2.4：更改输出字符串大小
       body.resize(fsize);

       //3.一次性读取整个文件的内容到字符串中
       /*
         文件流.read( 存放数据的地址, 要读多少字节 );
              第一个参数：&body[0]这是内存地址！
              第二个参数：fsize要读取的字节数（文件总大小）

         is_open ()：只负责一件事 → 文件有没有成功打开
         good ()：负责所有流状态 → 读、写、跳转、传输有没有出错
       */

       ifs.read(&body[0],fsize); //注意细节：ifs.read () 没有返回值（void）！它不返回成功 / 失败，也不返回读取了多少字节。
       if(ifs.good()==false)
       {
         ELOG("%s 文件读取失败",filename.c_str());
         ifs.close();
         return false;
       }
       //4.读取成功释放文件句柄
       ifs.close();
       return true;
   }
};
#endif

