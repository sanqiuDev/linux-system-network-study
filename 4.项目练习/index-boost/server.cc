#include "searcher.hpp"
#include "cpp-httplib/httplib.h"  // cpp-httplib轻量级HTTP服务器库（提供HTTP服务核心能力）


//定义全局变量
//1.预处理之后的文档的存放的路径
const std::string input =  "data/raw_html/raw.txt";
//2.HTTP服务器的根目录
const std::string root_path = "./wwwroot";


//程序主函数
int main()
{
    //1.初始化搜索器
    /*
        核心就是依托于seacher.hpp文件中的接口函数：“InitSearcher”
        参数：预处理后文档的存放路径
        返回值：void
    */
    ns_searcher::Searcher search;
    search.InitSearcher(input);


    //2.创建cpp-httplib服务器的实例
    httplib::Server srv;

    //3.设置静态资源目录
    svr.set_base_dir(root_path.c_str());
    /*
      一、它是干嘛的？（一句话）
        set_base_dir = 把本地文件夹映射成网站根目录，用来自动返回静态文件（HTML/CSS/JS/ 图片）
        它是 cpp-httplib 专用的静态文件托管函数。


    svr.set_base_dir(root_path.c_str());
        root_path = "./wwwroot"
        意思：把本地 ./wwwroot 文件夹，直接当成网站根目录 /
        效果：浏览器访问：http://127.0.0.1/index.html
        服务器自动去找：./wwwroot/index.html
        找到就返回 HTML，不用你写任何路由、不用你读文件、不用你发响应
        一句话：自动托管网页文件。


    四、和 Get () 的区别（你最关心）
        1. set_base_dir → 管静态文件（HTML/CSS/JS）
            自动找文件
            自动返回
            自动识别 MIME 类型
            不用写回调
        2. Get("/s", ...) → 管动态接口（搜索、登录、数据）
            你自己写逻辑
            你自己拿参数
            你自己生成结果
            必须写回调

       超级简单总结
            网页文件 → set_base_dir
            后台接口 → Get/Post

    
    */

    //4.注册GET请求路由：/s（搜索接口）
    /*
        svr.Get：注册一个网页接口
            /s：网址路径，比如 127.0.0.1/s
            [&search]：把搜索模块抓进来用
            req：用户发来的请求（包含关键词）
            rsp：我们要返回给用户的结果

        Get 这是 httplib 库的核心函数
            作用：注册一个网址接口
            大白话：用户访问网址 http://ip/s
            就会跑到这里来执行
            你可以理解成：给网站开一个门，门牌号是 /s


        参数 1："/s" → const char* path
            含义：路由路径（URL 路径）
            访问：http://127.0.0.1:端口/s
            类比：网站的 “地址门牌号”
            只能是字符串，固定路径
        参数 2：[&search](...) { ... } → 回调函数（F&& handler）
            这是一个 lambda 表达式，作为 “请求来了之后执行的函数”。
            [&search]：捕获外部变量（把外面的 search 对象拿进来用）
            (const httplib::Request& req, httplib::Response& rsp)：函数参数
                req：进来的请求（浏览器发过来的所有东西）
                rsp：要返回的响应（你要写给浏览器的东西）
            { ... }：函数体（你写的校验参数、拿 word、搜索、返回 JSON 全在这里）
    */
    srv.Get("/s",[&search](const httplib::Request& req,httplib::Response &rsp)
    {
        //4.1：校验请求参数，req中必须包含"word"参数
        if(!req.has_param("word"))
        {
            rsp.set_content("必须要有搜索关键词！","text/pain;charset=utf-8");
        }
        /*
        req.has_param("word")
            req = 用户发来的请求数据包
            函数意思：请求里有没有带 word 这个参数？
            比如用户访问：http://127.0.0.1/s?word=boost
            就有 word 参数 → 返回 true
            如果没写 ?word=xxx → 返回 false
            作用：判断用户有没有输入关键词
        */

        //4.2：提取搜索关键词
        std::string word = req.get_param_value("word");
        LOG(NORMAL, "用户搜索的: " + word); // 记录日志：打印用户搜索的关键词
        /*
          req.get_param_value("word")
            作用：从用户请求中，取出关键词
            比如用户访问：?word=boost
            这行代码就会拿到：std::string word = "boost";
        */


        //4.3：调用搜索的核心的逻辑，生成JSON格式的搜索结果
        /*
            核心就是依托于search.hpp文件中的接口函数：Search
            参数：
                1. 用户输入的关键词
                2. 存储生成的JSON的字符串
            返回值：void
        */
        std::string json_string;
        search.Search(word,&json_string);

        //4.4：构建HTTP响应
        rsp.set_connect(json_string,"application/json")
        /*
          rsp.set_content(内容, 类型)
            rsp = 我们要返回给浏览器的响应包
            作用：往浏览器里写内容
            两个参数：写什么内容 内容是什么类型（文本 / JSON / HTML）
            例子：rsp.set_content("必须输入关键字", "text/plain");
            浏览器就会显示：必须输入关键字
        */
    });
    LOG(NORMAL, "服务器启动成功..."); // 打印启动日志

    //5.启动服务器，监听指定地址和端口
    srv.listen("0.0.0.0",8080);
    return 0;
}



/*
二、再讲：为什么一个用 Get，一个用 set_http_handler？
这是两个不同库的设计风格，我给你讲透：
    1. 搜索引擎：cpp-httplib 库
    风格：简单、直接、像写网页 ---> svr.Get("/s", 回调);
    意思：访问 /s 路径，就调用这个回调，它是HTTP 专用库，专门做：网页，接口，短连接
    所以设计成：
        Get("/路径", 处理函数);
        Post("/路径", 处理函数);

    2. 五子棋：websocketpp 库
    风格：底层、通用、全能 ---> websocketpp 是全功能 WebSocket 服务器
    它不只处理 HTTP，还处理：WebSocket 长连接
        连接打开
        连接关闭
        消息收发
    所以它的设计是：
        set_http_handler(处理HTTP的函数);
        set_open_handler(连接打开);
        set_close_handler(连接关闭);
        set_message_handler(消息到来);
    意思：
        HTTP 请求来了 → 调用 http_callback
        打开连接 → 调用 open
        收到消息 → 调用 message
        它不提供直接 Get/Post，因为太高级了，它是底层库！
*/