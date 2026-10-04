#ifndef SERVER_HPP
#define SERVER_HPP

#include "db.hpp"
#include "online.hpp"
#include "room.hpp"

#include "session.hpp"
#include "matcher.hpp"

//先定义一些服务器的全局常量配置
//1.静态资源根目录
#define WWWROOT "./wwwroot/"


//在创建：“五子棋服务器的核心服务类：class gobang_server”
class gobang_server
{
    private:
    /*
       1. std::string _web_root; 作用：放网页、前端文件的目录  没有它 → 玩家打不开网页！
       2. wsserver_t _wssrv;     作用：WebSocket 服务器本体   没有它 → 服务器根本跑不起来！
       3. user_table _ut;        作用：数据库（用户信息）      没有它 → 玩家不能登录，没有战绩！
       4. online_manager _om;    作用：管理谁在线、谁掉线      没有它 → 不知道玩家在不在！
       5. room_manager _rm;      作用：管理所有房间           没有它 → 无法开局！
       6. matcher _mm;           作用：匹配玩家               没有它 → 玩家无法匹配对手！
       7. session_manager _sm;   作用：会话管理（登录状态）    没有它 → 玩家每次操作都要重新登录！
    */
   //1.静态资源目录
   std::string _web_root;
   //2.websocket服务器对象
   wsserver_t _wssrv;

   //3.用户表操作对象
   user_table _tb_user;
   //4.在线管理对象
   online_manager _online_user;
   //5.房间管理对象
   room_manager _room_user;

   //6.会话管理对象
   session_manager _session_user;
   //7.匹配管理对象
   matcher _matcher_user;

   /*就是之前我们在定义其他类的对象时候都是定义对象指针啊，这里怎么都是定义的对象啊?
     到底什么时候用 对象？什么时候用 指针？我用最简单、最直白、最贴近你们项目的方式给你讲透👇
     终极结论（先背下来）
        谁是 “总老板”，谁就直接持有【对象】，被调用的 “员工”，才用【指针】传递 
     你们的架构是：
           gobang_server（总老板）
                ↓ 管理所有模块
    user_table / online_manager / room_manager / matcher / session_manager（全是员工）
   */
    public:
    //1.实现：“HTTP响应封装工具函数”
    void http_resp(
        wsserver_t::connection_ptr &conn,
        websocketpp::http::status_code::value code,
        bool result,
        const std::string &reason
    )
    /*
        发消息必须要有连接 → 必须传 conn
        成功 / 失败 → 必须传 bool result
        HTTP 必须有状态码 → 必须传 code
        要给用户提示 → 必须传 reason
    */
   {
      //1.创建Json::Value响应对象
      Json::Value json_resp;
      json_resp["result"]=result;
      json_resp["reason"]=reason;

      //2.序列化Json::Value对象为JSON字符串
      std::string body;
      json_util::serialize(json_resp,body);

      //3.设置HTPP响应的状态行，响应头和响应体
      conn->set_body(body);
      conn->set_status(code);
      conn->append_header("Content-Type","application/json");
      return;

    /*
       终极答案：HTTP 没有 send () 这种发送方法！
       HTTP 根本不用 send ()！HTTP 只用 set_body () + 自动发送！


       1. 为什么 HTTP 没有 send ()？
            因为 HTTP 设计规则就是：一次请求 → 一次响应 → 直接断开，服务器必须一次性把所有数据给完
            所以：HTTP 不需要你手动调用 send()，你只需要 填充响应内容，框架会自动帮你发送，然后断开连接

        // HTTP 响应：只设置，不 send
            conn->set_status(200);
            conn->append_header("Content-Type", "application/json");
            conn->set_body("{...}");

            // 没有 conn->send()！！！
            // 函数结束，框架自动发送！

        最核心区别（背会这张表）
        协议	    怎么发数据	  是否需要调用    send ()	特点
        HTTP	    set_body()	 不需要         一次发送，自动断开
        WebSocket	send()	     必须手动调用	可多次发送，长连接

        HTTP：只管 “填数据”，不用管 “序列化” 和 “发送”！
        WebSocket：必须自己 “序列化 + 手动 send”！
    */


      /*
        append_header = 给 HTTP 响应加一个 “头信息”，作用：告诉浏览器，我返回给你的内容是什么格式！

            1. 先看你这行代码
                conn->append_header("Content-Type", "application/json");
                意思：我给你返回的内容格式是 JSON！
                浏览器收到后，就知道：“哦！这是 JSON 数据，我要按 JSON 解析！”

            2. 什么是 HTTP 头（Header）？HTTP 响应分为三部分：
                    状态行 → set_status
                    响应头 → append_header（说明信息）
                    响应体 → set_body（真正内容）
                    
                头信息 = 给浏览器的 “说明书”告诉浏览器：
                    内容是什么格式
                    用什么编码
                    要不要缓存

        append_header 到底干嘛？
        函数功能：添加一个键值对格式的响应头，格式固定：append_header(键, 值);
            append_header = 添加 HTTP 头信息
            Content-Type = 告诉浏览器返回内容是什么格式
            application/json = 我返回的是 JSON！
      */


      /*
         一些扩展的细节：
            file_handler 返回的是【网页 / HTML】，浏览器自动认识 
            http_resp 返回的是【JSON 数据】，浏览器不认识，必须告诉它 

         因为 websocketpp 库会自动帮你加！
            你返回的是 .html 文件，库默认设置：Content-Type: text/html
            浏览器天生就懂网页，所以：返回网页 → 不用手动加 Header！ 
      */
   }




    //2.实现：“HTTP回调——静态资源请求处理”
    void file_handler(wsserver_t::connection_ptr&conn)
    //本质上这是一个：服务器和客户端的交互
    //所以服务器想要和客户端进行任何交互都要传入wsserver_t::connection_ptr
    {
        //1.获取HTTP请求对象并提取URL
        /*
            1. conn->get_request() 意思：拿到客户端发来的整个 HTTP 请求
                客户端访问网页、登录、注册 → 发的是 HTTP 请求
                get_request() 就是把一整包 HTTP 数据取出来，
                返回值是：websocketpp::http::parser::request
                你就理解成：get_request () = 拿到客户端的 “请求信封”

            2. req.get_uri() 意思：从请求里拿到 “地址路径”
                URI 就是客户端访问的网址路径
                req.get_uri() = 拿路径，判断客户端想干嘛！
        */
        websocketpp::http::parser::request req=conn->get_request();
        std::string uri=req.get_uri();
        /*
            req 就是：浏览器发给服务器的【完整 HTTP 请求数据包】！
            一句话终极结论：req = 完整的 HTTP 请求
            里面装着：请求行 + 请求头 + 请求体，你想从请求里拿任何东西，都从 req 里取！
            
            最直白的解释：req 就是 把浏览器发过来的一整段 HTTP 请求，解析成一个对象。

            它里面完整包含了浏览器告诉你的所有信息：
                请求方法（GET / POST）
                请求路径（/login / /info）
                HTTP 版本
                所有请求头（Cookie、Token、Content-Type...）
                请求体（POST 提交的账号、密码...）
        
            HTTP 请求长啥样？
                POST /login HTTP/1.1          <-- 请求行
                Host: localhost:8080          <-- 请求头
                Cookie: SSID=123456           <-- 请求头
                Content-Type: application/json

                {"username":"zhangsan","password":"123456"}  <-- 请求体
        */

        //2.拼接文件实际的路径
        std::string realpath = _web_root+uri; 

        //3.处理目录请求
        /*
            为什么要这么做？因为：
                用户访问网站不可能手动输 login.html
                访问 ip:port 就应该直接看到登录页，这是网站的默认规则
        */
       if(realpath.back()=='/')
       {
         realpath+="login.html";
       }

       //4.读取文件的内容
       std::string body;
       bool ret=file_util::read(realpath,body);
       
       //5.文件不存在返回404响应
       if(ret==false)
       {
         //5.1：构造404页面
         /*
             <html>
                 <head>
                    这里放设置信息
                </head>
                <body>
                    这里放你看到的内容
                </body>
            </html>
         */
         body+="<html>";
         body+="<head>";
         body+="<meta charset='UTF-8'/>";
         /*
            这个斜杠 / 表示：标签自己关闭，不需要写结束标签！ 
            但 <meta> 这种标签 没有内容，它只是一个配置、设置，里面不放东西。
            所以它可以自己关闭自己，写法就是：<meta ...  />
            斜杠 / 的意思就是：我这个标签到此结束，不用写 </meta>！
            
            下面两种写法完全一样，作用相同：
                <meta charset="UTF-8">           <!-- 普通写法 -->
                <meta charset="UTF-8" />         <!-- 严谨/XHTML写法 -->
            斜杠只是一个规范写法，可加可不加！加了更严谨，不加浏览器也认识。
        */
         body+="</head>";

         body+="<body>";
         body+="<h1> Not Found</h1>";
         body+="</body>";

         //5.2：设置HTTP状态码：404
         /*
            1. conn->set_body(body); 作用：设置响应的 “内容” “这是我要给你的内容！”
            2. conn->set_status(状态码); 作用：告诉浏览器这次请求 “成没成功”
         */
          conn->set_body(body);
          conn->set_status(websocketpp::http::status_code::not_found);
       }

       //6.文件存在返回200响应
       conn->set_body(body);
       conn->set_status(websocketpp::http::status_code::ok);

       /*
          它们只做这种：set_body，set_status这种set的操作，真的会返回给发送http请求的客户端吗？
          我感觉要应该需要一个发送的操作啊，怎么没有呢？

           不需要你手动写 send！不需要你手动写返回！
           核心真相（一句话击穿）：你在回调函数里做的所有 set_xxx 都是在 “打包快递”，等你函数一结束，库自动帮你发出去！
       
           那库怎么知道要调用它？因为你 在初始化服务器的时候，注册过它！
            代码一定长这样：
            // 初始化 HTTP 服务器
            wsserver_.set_http_handler(
            std::bind(
                &gobang_server::file_handler,  // ← 你告诉库：调用我这个函数！
                this,
                std::placeholders::_1
            ));
            看到了吗？你在这里告诉库：当 HTTP 请求来的时候 → 调用 file_handler！
       */
    }

    //3.实现：“HTTP回调——用户注册请求处理”
    void reg(wsserver_t::connection_ptr &conn)
    {
        //1.获取客户端通过HTTP发过来的请求数据
        std::string req_body=conn->get_request_body();
        /*
            send () 是 WebSocket 用的，get_request_body () 是 HTTP 用的
            你们项目里：
                WebSocket 消息 → 不走这个函数
                HTTP 请求（登录 / 注册 / 获取信息）→ 才用这个函数
            
            ✔ conn->send()→ 给客户端发消息（WebSocket 下棋 / 匹配 / 聊天）
            ✔ conn->get_request_body()→ 拿客户端上传的数据（HTTP 登录 / 注册）

        // websocketpp::http::parser::request req = conn->get_request();
        // std::string req_body = rep->get_body();
        注意：上面的写法是错误的，因为：
        *    1. 这个 req 里面 只有请求头信息
        *    2. 请求方法（GET/POST）
        *    3. 请求路径（/reg/login）
        *    4. 请求头
        *  它不包含请求体 body！它没有 get_body () 这个函数！
    
        */

        //2.反序列化请求正文
        Json::Value login_info;
        bool ret=json_util::unserialize(req_body,login_info);
        if(ret==false)
        {
            DLOG("反序列化失败");

            //构造400http报文
            return http_resp(conn,websocketpp::http::status_code::bad_request,false,"请求正文格式错误");
            /* 这里讲解一个小细节就是：可能的你输入的"请求正文格式错误"是报错的
            这是为什么呢？
                你的 http_resp 函数，最后一个参数的类型是 std::string&（非常量引用），
                但你传的是 "请求正文格式错误"，它的真实类型是 const char[25]（字符串常量）。
                C++ 不允许用一个临时的常量字符串，去初始化一个非常量引用。
            不能把一个临时的常量值，绑定到非 const 的引用上。因为编译器会认为：
            你既然传了一个临时常量，函数却声明了非常量引用，意味着你可能会修改它，
            但临时常量是不可修改的，存在矛盾，所以直接报错。

            所以你需要：修改http_resp函数定义，把参数改成 const std::string&
            */
        }   

        //3.校验用户名密码的完整性
        if(login_info["username"].isNull()||login_info["passeord"].isNull())
        {
            DLOG("用户名或密码不能为空");

            //构造400http报文
            return http_resp(conn,websocketpp::http::status_code::bad_request,false,"请输入用户名/密码");
        }

        //4.向数据库中插入新用户
        if(_tb_user.insert(login_info)==false)
        {
            DLOG("新用户的插入操作失败");

            //设置失败的响应
            return http_resp(conn,websocketpp::http::status_code::bad_request,false,"用户名已被占用");
        }

        //5.注册成功并设置响应
        return http_resp(conn,websocketpp::http::status_code::ok,true,"用户注册成功");
    }

    //4.实现：“回调函数——用户登录请求处理”
    void login(wsserver_t::connection_ptr &conn)
    {
        //1.获取http请求并提取请求中的正文
        std::string req_body=conn->get_request_body();

        //2.将请求正文进行反序列化
        Json::Value login_info;
        bool ret = json_util::unserialize(req_body,login_info);
        if(ret ==false)
        {
            DLOG("请求正文的反序列化失败");
            return http_resp(conn,websocketpp::http::status_code::bad_request,false,"请求的正文的格式错误");
        }

        //3.校验用户名和密码的完整性
        if(login_info["username"].isNull()||login_info["password"].isNull())
        {
            DLOG("用户输入的用户名和密码不完整");

            return http_resp(conn,websocketpp::http::status_code::bad_request,false,"请输入完整的用户名和密码");
        }

        //4.使用数据库校验用户名和密码的正确性
        if(_tb_user.login(login_info)==false)
        {
            DLOG("登录认证失败");

            return http_resp(conn,websocketpp::http::status_code::bad_request,false,"用户名/密码输入错误，请重新输入");
        }

        //5.创建会话
        uint64_t uid = login_info["id"].asUInt64();
        std::shared_ptr<session> ssp = _session_user.create_session(uid,LOGIN);
        //因为你在当前文件的最上面，#include 了 "session.hpp"！所以里面的 类、枚举、函数，你全都能直接用！
        if(ssp==nullptr)
        {
            DLOG("会话创失败");

            return http_resp(conn,websocketpp::http::status_code::bad_request,false,"会话创建失败");
        }

        //6.设置session的超时时间
        _session_user.set_session_expire_time(ssp->get_ssid(),SESSION_TIMEOUT);

        //7.设置cookie将其返回给客户端
        /*
            给客户端浏览器种下一个 Cookie：SSID = 会话 ID
            下次浏览器再来请求，自动带上这个 Cookie
            服务器就知道：哦！是你！我认识你！


            append_header = 往 HTTP 响应的 “头部信息” 里添加一条数据
            HTTP 响应长这样：
                HTTP/1.1 200 OK              <-- 状态（set_status）
                Content-Type: text/html      <-- 头部1（append_header）
                Set-Cookie: SSID=123456      <-- 头部2（append_header）
                （空行）
                网页内容/json内容             <-- 身体（set_body）
                append_header 就是添加中间这些 键：值 说明信息
        */
        std::string cookie_ssid= "SSID="+std::to_string(ssp->get_ssid());
        conn->append_header("Set-Cookie",cookie_ssid);


        //8.成功登陆设置响应
        return http_resp(conn,websocketpp::http::status_code::bad_request,false,"用户登录成功");
    }


    //5.实现：“从Cookie字符串中提取指定Key的值”
    //从上面的介绍我们不难看出的一点就是：我们需要传入的参数是（Cookie字符串，键值对中的key，键值对中的value）
    bool get_cookie_val(const std::string &cookie_str,const std::string &key,std::string &value)
    {
        //1.以分隔符;将Cookie字符串分割成单个Cookie键值对
        std::string sep=";";
        std::vector<std::string> cookie_arr;
        string_util::split(cookie_str,sep,cookie_arr);

        //2.遍历所有的Cookie键值对并将其键/值分割，提取对应键的值
        for(std::string it :cookie_arr)
        {
            //2.1：以分隔符=将单个Cookie键值对分割成键和值
            std::vector<std::string> tmp_arr;
            string_util::split(it,"=",tmp_arr);

            //2.2：格式错误（无=或者多个=）跳过这个循环
            if(tmp_arr.size()!=2) {continue;}

            //2.3：匹配目标key提取对应的value
            if(tmp_arr[0]==key)
            {
                value=tmp_arr[1];
                return true;
            }
        }

        //3.没有找到的目标
        return false;
    }


    //6.实现：“HTTP回调函数——用户信息查询请求处理”
    /*
       先自己进行分析一下要怎么写吧：db.hpp文件中有一个函数是：select_by_id（作用是根据用户ID获取用户的详细的信息）
          1. 获取用户ID 
          2. 调用该函数
       上面这种思路不能直接获取用户ID，要升级获取用户ID的方式
    */
    void info(wsserver_t::connection_ptr &conn)
    {
        //1.获取Cookie字符串
        std::string cookie_str=conn->get_request_header("Cookie");
        if(cookie_str.empty())
        {
            DLOG("Cookie字符串获取失败");
            return http_resp(conn,websocketpp::http::status_code::bad_request,false,"找不到Cookie信息，请重新登录");
        }

        //2.从Cookie中提取会话ID
        std::string value;
        if(get_cookie_val(cookie_str,"SSID",value)==false)
        {
            DLOG("从Cookie中提取会话ID失败");
            return http_resp(conn,websocketpp::http::status_code::bad_request,false,"找不到ssid信息，请重新登录");
        }

        //3.通过会话ID获取会话对象
        std::shared_ptr<session> ssp =_session_user.get_session_by_ssid(std::stol(value));
        if(ssp==nullptr)
        {
            DLOG("通过会话ID获取会话对象失败");
            return http_resp(conn,websocketpp::http::status_code::bad_request,false,"登录过期，请重新登录");
        }

        //4.查询用户信息
        uint64_t uid=ssp->get_uid();
        Json::Value user_info;
        bool ret= _tb_user.select_by_id(uid,user_info);
        if(ret==false)
        {
            DLOG("查询用户信息失败");
            return http_resp(conn,websocketpp::http::status_code::bad_request,false,"找不到用户信息，请重新登录");
        }

        //5.序列化用户信息并设置响应
        std::string body;
        json_util::serialize(user_info,body);
        conn->set_status(websocketpp::http::status_code::ok);
        conn->append_header("Content-Type","application/json");
        conn->set_body(body);

        //6.刷新Session的超时时间
        _session_user.set_session_expire_time(std::stoi(value),SESSION_TIMEOUT);
        /*
             为什么最后要 刷新 Session 超时时间？
            一句话答案：只要用户还在操作页面，就不能让他掉线！
            所以每次用户发请求 → 刷新超时时间 → 重新计时 30 秒这就叫 “活动续期”
        */
    }

    //7.实现：“HTTP请求分发器”
    void http_callback(websocketpp::connection_hdl hdl)
    /*
        hdl = 连接句柄（handle）
        它就是当前客户端与服务器之间的 “连接唯一标识”，相当于连接的 “身份证”

        1. 它到底是什么？（底层本质）
            connection_hdl 本质是一个 弱智能指针（weak_ptr）它不管理内存，只用来表示 “哪一个连接”。
            你可以把它理解成：一个用来找到对应连接的 “号码牌”

        2. 它和你之前用的 connection_ptr 有什么关系？
            你之前见的是：void info(wsserver_t::connection_ptr& conn)
            这两个的关系是：
                connection_hdl = 连接的 “编号”
                connection_ptr = 连接的 “完整对象”

        3. 为什么有的函数用 hdl，有的用 conn？
            ① HTTP 请求（你之前写的）
                void file_handler(wsserver_t::connection_ptr &conn)
                HTTP 是短连接，服务器需要直接操作响应：set_status、set_body
                所以给你 完整连接对象 conn
            ② WebSocket 连接 / 断开 / 消息（现在这个）
                void on_open(websocketpp::connection_hdl hdl)
                void on_close(websocketpp::connection_hdl hdl)
                WebSocket 是长连接，这些回调只需要知道 “谁连接 / 断开了”，不需要直接发 HTTP 响应，
                所以只给你 连接编号 hdl

    
        4. 有了 hdl，我们能干嘛？最常用 3 个用途：
            （1）根据 hdl 获取连接对象
            wsserver_t::connection_ptr conn = _server.get_con_from_hdl(hdl);

            （2）给这个客户端发送消息
            _server.send(hdl, "hello", websocketpp::frame::opcode::text);

            （3）保存这个 hdl，用来管理在线用户
            _connections.insert(hdl);


        5. 超级直白比喻
            connection_hdl = 电话号码
            connection_ptr = 手机本身
        想打电话、发短信 → 只要电话号码（hdl）
        想设置手机、改配置 → 需要手机本身（conn）
    */
    {
        //1.获取连接指针
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
        /*
            根据连接编号（hdl），把完整的连接对象（connection_ptr）取出来！
            我不明白的是为什么是_wssrv调用get_con_form_hdl这个函数可以获得这个完整的连接对象?
            终极核心答案：
                因为所有连接，全都存在 _wssrv（服务器对象）里面！
                只有服务器自己，才知道每个编号对应哪个连接！
            
            我用最通俗的方式讲（你秒懂）想象：
                _wssrv = 学校（服务器）
                hdl = 学号（连接编号）
                conn = 学生本人（连接对象）
            学校里，所有学生都存在学校里，你现在手里只有一个学号 hdl你想找到这个学生本人，必须问学校！
                学校.通过学号找学生(学号);


                1. 当客户端连进来，
                    服务器 _wssrv 会创建一个连接对象 conn
                    把它保存到自己的内部哈希表 / 映射表中
                    给你一个编号 hdl
                2. 服务器内部大概长这样
                    _wssrv 的内部：
                    连接表：
                        编号1 → 连接对象A
                        编号2 → 连接对象B
                        编号3 → 连接对象C
        */

        //2.获取请求方法和和URL
        websocketpp::http::parser::request req=conn->get_request();
        std::string method = req.get_method();
        std::string uri = req.get_uri();

        //3.分发请求
        if(method=="POST"&&uri=="/reg")
        {
            return reg(conn);
        }
        else if(method=="POST"&&uri=="/login")
        {
            return login(conn);
        }
        else if(method=="GET"&&uri=="/info")
        {
            return info(conn);
        }
        else 
        {
            return file_handler(conn);
        }
    }
    /*
        1. 先看：wsserver_t::connection_ptr，全称：连接智能指针
                来自 HTTP 请求
                你用它来：
                    set_status()
                    set_body()
                    append_header()
                作用：给浏览器返回 HTTP 响应，你之前写的登录、注册、info 接口全用它

        2. 再看：websocketpp::connection_hdl，全称：连接句柄（编号）
                来自 WebSocket 长连接
                你用它来：
                    send(hdl, msg) 给客户端发消息，标识哪个客户端在线
                作用：WebSocket 双向聊天、游戏通信
    
        它们是同一个连接的两种表示！
        流程是这样的：
            浏览器发起 HTTP 请求→ 你拿到 connection_ptr
            浏览器发送 WebSocket 握手请求（也是 HTTP）→ 协议升级
            连接升级成 WebSocket 长连接→ 你拿到 connection_hdl
        所以：
            connection_ptr 是 HTTP 阶段用的 “完整对象”
            connection_hdl 是 WebSocket 阶段用的 “编号”
    */



    //8.实现：“HTTP响应封装工具函数”
    /* 首先想清楚我们需要什么字段的内容？
         1. 智能连接指针
         2. 响应的类型
         3. 响应的结果
         4. 响应的原因
    */
   /*
    void ws_resp(wsserver_t::connection_ptr &conn,Json::Value &resq)
    {
        std::string body;
        json_util::serialize(resq,body);
        conn->send(body);
    }
    */

    //升级写法
    void ws_resp(
        wsserver_t::connection_ptr &conn,
        const std::string &optype,
        bool result,
        const std::string &reason
    )
    {
        //1.构建Json::Value响应对象
        Json::Value resp_json;
        resp_json["optype"]=optype;
        resp_json["result"]=result;
        resp_json["reason"]=reason;

        //2.序列化Json::Value对象为JSON字符串
        std::string body;
        json_util::serialize(resp_json,body);

        //3.发送JSON字符串
        conn->send(body);
    }
    /*
        终极核心结论（背会这 2 条，一辈子不乱）
            1. HTTP 接口（登录、注册、info）
                不需要响应类型必须设置请求头自动发送
            2. WebSocket 业务（大厅、游戏、消息）
                必须写响应类型（optype）不用设置任何头必须手动 send
    */


    //9.实现：“websocket通用工具通过cookie获取session对象”
    std::shared_ptr<session> get_session_from_cookie(wsserver_t::connection_ptr &conn)
    {
        //1.从连接智能指针中获取Cookie字符串
        std::string cookie_str=conn->get_request_header("Cookie");
        if(cookie_str.empty())
        {
            DLOG("Cookie字符串获取失败");
            //return http_resp(conn,websocketpp::http::status_code::bad_request,false,"找不到Cookie信息，请重新登录");
            ws_resp(conn,"hall_ready",false,"找不到Cookie信息，请重新登录");
            return std::shared_ptr<session>();

            /*为什么常量字符串
                可以给std::string str  和 const std::string &str 赋值
                但是不能给std::string &str赋

              终极一句话结论
                    1. 非常量引用（&）只能引用 “活人”（变量），不能引用 “死人”（临时量 / 常量）！
                    2. 常量引用（const &）既能引用活人，也能引用死人！
                
                非常量引用（&）代表：我可能要修改它！你要修改一个临时 / 常量？→ 编译器直接禁止！
            */
        }

        //2.从Cookie字符串中获取会话ID
        std::string value;
        if(get_cookie_val(cookie_str,"SSID",value)==false)
        {
            DLOG("从Cookie中提取会话ID失败");
            //return http_resp(conn,websocketpp::http::status_code::bad_request,false,"找不到ssid信息，请重新登录");
            ws_resp(conn,"hall_ready",false,"找不到ssid信息，请重新登录");
            return std::shared_ptr<session>();

            /*
               "hall_ready"，给前端回复一条【大厅准备】的消息，告诉它：登录失效了！
               
               最直白总结（背会这句）
                    hall_ready = 前后端约定的业务代号
                    含义：【游戏大厅初始化 / 准备就绪】
                    作用：让前端知道收到消息后该做什么 
            */
        }

        //3.通过会话ID获取会话对象
        std::shared_ptr<session> ssp = _session_user.get_session_by_ssid(std::stol(value));
        if(ssp==nullptr)
        {
            DLOG("会话对象获取失败");
            ws_resp(conn,"hall_ready",false,"没有找到session信息，需要重新登录");
            return std::shared_ptr<session>();
        }
        return ssp;
        
    }

    //10.实现：“websocket回调函数——游戏大厅建立的处理”
    /*
        首先这里我们需要先整理一下思路：
            这个很容能想到的是online.hpp文件中的有一个函数：enter_game_hall(用户ID，智能连接指针)
            所以任务就变成了我们要怎么获得？
                1. 用户ID
                2. 智能连接指针 （已经拥有）
        智能连接指针 -> Cookie字符串 -> 会话ID -> 会话对象 -> 用户ID
        get_session_from_cookie()  -----------> 会话对象 -> 用户ID
    */
    void wsopen_game_hall(wsserver_t::connection_ptr &conn)
    {
        //1.获取该连接对应是session对象
        std::shared_ptr<session> ssp = get_session_from_cookie(conn);
        if(ssp==nullptr)
        {
            return ws_resp(conn,"hall_ready",false,"获取会话对象失败");
        }

        //2.通过会话对象获取对应的用户ID并验证该玩家是否已经在游戏大厅/房间
        uint64_t uid = ssp->get_uid();
        if(_online_user.is_in_game_hall(uid)&&_online_user.is_in_game_room(uid))
        {
            return ws_resp(conn,"hall_ready",false,"玩家重复登录");
        }

        //3.用户进入游戏大厅
        _online_user.enter_game_hall(uid,conn);
        return ws_resp(conn,"hall_ready",true,"游戏大厅连接建立成功");

        //4.设置Session永久存在
        _session_user.set_session_expire_time(ssp->get_ssid(),SESSION_FOREVER);

        /*
            什么时候必须更新 Session 超时？
            我给你列最清晰、最真实、项目里真正用的 3 种场景：
                场景 1：客户端刚登录成功
                登录成功 = 新创建 Session→ 必须设置超时时间

                场景 2：客户端发送了任何消息 / 操作
                比如：
                    进入大厅
                    发送聊天
                    开始游戏
                    点击按钮
                    心跳包
                    → 只要客户端动了，就刷新超时时间！

                场景 3：你想让他永久在线（比如游戏大厅）
                    就像你现在代码：SESSION_FOREVER→ 游戏在线期间，永久不踢下线
        */
    }

    //11.实现：“websocket回调函数——游戏房间的连接处理”
    //很明显这个函数的实现基本上和上面的那个函数是一样的，所以这也是我们为什么要去封装get_session_from_cookie这个函数的原因
    void wsopen_game_room(wsserver_t::connection_ptr &conn)
    {
        //1.获取该连接对应是session对象
        std::shared_ptr<session> ssp = get_session_from_cookie(conn);
        if(ssp==nullptr)
        {
            return ws_resp(conn,"room_ready",false,"获取会话对象失败");
        }

        //2.通过会话对象获取对应的用户ID并验证该玩家是否已经在游戏大厅/房间
        uint64_t uid = ssp->get_uid();
        if(_online_user.is_in_game_hall(uid)&&_online_user.is_in_game_room(uid))
        {
            return ws_resp(conn,"room_ready",false,"玩家重复登录");
        }

        //3.用户进入游戏房间
        _online_user.enter_game_room(uid,conn);
        return ws_resp(conn,"room_ready",true,"游戏大厅连接建立成功");

        //4.设置Session永久存在
        _session_user.set_session_expire_time(ssp->get_ssid(),SESSION_FOREVER);
    }


    //12.实现：“websocket连接建立回调分发器”
    void wsopen_callback(websocketpp::connection_hdl hdl)
    {
        //1.通过连接句柄获取连接指针
        wsserver_t::connection_ptr conn=_wssrv.get_con_from_hdl(hdl);

        //2.通过连接指针获取websocket请求对象，并提取URL
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri=req.get_uri();

        //3.分发请求
        if(uri=="/hall")
        {
            return wsopen_game_hall(conn);
        }
        else if(uri=="/room")
        {
            return wsopen_game_room(conn);
        }
    }

    //13.实现：“websocket回调函数——游戏大厅断开连接处理”
    //通过上面的练习后，针对于现在的这个函数的实现我们已经知道了核心是调用：exit_game_hall
    //缺少什么，怎么获得和上面的如出一辙
    void wsclose_game_hall(wsserver_t::connection_ptr &conn)
    {
        //1.通过智能连接指针获取会话对象
        std::shared_ptr<session> ssp=get_session_from_cookie(conn);
        if(ssp==nullptr)
        {
            // return ws_resp(conn,"room_ready",false,"获取会话对象失败");
            return ;
        }

        //2.通过会话对象获取玩家的ID并直接将玩家从游戏大厅移除
        _online_user.exit_game_hall(ssp->get_uid());
        /*
        uint64_t uid=ssp->get_uid();
        if(_online_user.is_in_game_hall(uid)==false&&_online_user.is_in_game_room(uid)==false)
        {
            return ws_resp(conn,"room_ready",false,"玩家重复登录");
        }
        _online_user.exit_game_hall(uid);
        return ws_resp(conn,"hall_ready",true,"退出游戏大厅成功");
        */

        /*
            这里不是不能写，而是：根本不需要写！
            客户端主动关闭 WebSocket 时，服务器不需要、也不能回消息！我给你讲得明明白白👇
                
            1. 这个函数什么时候被调用？
                void wsclose_game_hall(...)
                这是：客户端关闭 WebSocket 连接 的回调！
                场景：
                    玩家关闭网页
                    玩家刷新页面
                    玩家跳转到其他页面
                    连接断开
                    → 这时候，连接已经关闭 / 正在关闭！
            
            2. 为什么不能 / 不需要发送响应？
                ① 连接已经断了，你发了对方也收不到
                    浏览器都关了 / 断开了你再 ws_resp 发消息等于给空气说话没用，还会报错！
                ② WebSocket 关闭协议规定：关闭时不用回复业务消息
                    关闭就是关闭不需要返回 optype不需要返回成功失败
                ③ 这是服务器做 “善后清理”
                    你只需要干这些事：
                        从大厅移除玩家
                        恢复 Session 超时
                        释放资源
                        不需要给前端回复任何消息！
        */

        //4.更新会话的超时时间
        _session_user.set_session_expire_time(ssp->get_ssid(),SESSION_TIMEOUT);
    }

    //14.实现：“websocket的回调函数——游戏房间退出处理”
    void wsclose_game_room(wsserver_t::connection_ptr &conn)
    {
        //1.获取会话对象
        std::shared_ptr<session> ssp = get_session_from_cookie(conn);
        if(ssp==nullptr)
        {
            return ;
        }
        //2.更新玩家在线状态：退出游戏房间（回到大厅状态）
        _online_user.exit_game_room(ssp->get_uid());

        //3.恢复session的超时时间
        _session_user.set_session_expire_time(ssp->get_ssid(),SESSION_TIMEOUT);

        //4.更新房间成员信息：从房间内移除该玩家（无人房间自动销毁）
        _room_user.remove_room_user(ssp->get_uid());

        /*
        第二步 和 第四步 做的不是同一件事！
        一个管【在线状态】，一个管【房间数据】，职责完全分开！
            1. 先看第二步：
                    _om.exit_game_room(ssp->get_user());
                    _om = OnlineManager（在线用户管理器）
                它的职责：只管 “这个玩家当前在哪个场景”
                    大厅？房间？离线？
                所以：第二步 = 更新玩家自己的状态
                告诉系统：这个玩家，现在不在房间里了！回到大厅 / 离线状态！

            2. 再看第四步：
                    _rm.remove_room_user(ssp->get_user());
                    _rm = RoomManager（房间管理器）
                它的职责：只管 “房间里有谁、房间数据”
                        房间里的玩家列表，房主是谁，房间状态，房间是否需要销毁
                    所以：
                    第四步 = 更新房间的数据
                    告诉房间：这个玩家走了，你把他从你的成员列表删掉！
                    如果没人了，你就销毁自己！
        

        终极一句话结论：
            游戏大厅不需要 _rm（房间管理器），因为大厅是公共区域，不是房间！
            所以大厅断开时，只需要清理玩家自己的状态，不需要清理房间！
        
        我用最简单的方式给你讲透
            1. 游戏大厅 = 公共广场（所有人都在这）
                大厅 = 所有人都能进的公共场所
                没有“房间”概念
                没有“房间列表”
                没有“销毁大厅”这种说法
              所以：
                玩家退出大厅
                只需要修改玩家自己的状态
                不需要去 “大厅” 里删除他
                因为大厅根本不是房间
        */
    }

    //15.实现：“websocket连接断开的回调分发器”
    void wsclose_callback(websocketpp::connection_hdl hdl)
    {
        //1.获得智能连接指针
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);

        //2.获取请求对象并提取url
        websocketpp::http::parser::request req = conn->get_request();
        std::string url = req.get_uri();

        //3.请求分发
        if(url=="/hall")
        {
            return wsclose_game_hall(conn);
        }
        else if(url=="/room")
        {
            return wsclose_game_room(conn);
        }

    }

    //16.实现：“websocket回调函数——游戏大厅消息的处理”
    void wsmsg_game_hall(wsserver_t::connection_ptr &conn,wsserver_t::message_ptr &msg)
    /*
        先看你问的这个：wsserver_t::message_ptr msg
        一句话理解：msg = 浏览器发给服务器的【消息内容】！
    
        详细解释：当客户端通过 WebSocket 发送：{ "optype": "match_start" }
        服务器收到的整段数据，就被封装在这个 msg 对象里。
        你用它来干嘛？取消息正文！
        拿到客户端发过来的字符串 std::string body = msg->get_payload(); 
        这就是它唯一的作用！
    */
    {
        //1.通过智能连接指针获取会话对象，做身份验证
        std::shared_ptr<session> ssp = get_session_from_cookie(conn);
        if(ssp ==nullptr)
        {
            return;
        }

        //2.提取消息对象中的消息正文并对其进行反序列化
        Json::Value resp_body;
        std::string body = msg->get_payload();
        bool ret = json_util::unserialize(body,resp_body);
        if(ret==false)
        {
            return ws_resp(conn,"",false,"请求信息解析失败");
        }

        //3.处理匹配请求
        std::string optype = resp_body["optype"].asString();
        //3.1：情况一：开始匹配请求
        if(optype=="matcher_start")
        {
            _matcher_user.add(ssp->get_uid());
            return ws_resp(conn,"matcher_start",true,"");
        }

        //3.2：情况二：停止匹配请求
        if(optype=="matcher_stop")
        {
            _matcher_user.del(ssp->get_uid());
            return ws_resp(conn,"matcher_stop",true,"");
        }

        //3.3：情况三：未知请求
        return ws_resp(conn,"unknow",false,"请求类型未知");
    }

    /*终极分类：所有类型分 4 组，瞬间不乱！
        
        第一组：连接相关（管 “通道”）
            1. wsserver_t::connection_ptr conn = 连接对象（完整手机）
                用来发送消息、获取请求，操作这条连接
            
            2. websocketpp::connection_hdl hdl = 连接编号（手机号）
                只是个标识，用来查找到底是哪个客户端

        第二组：消息相关（管 “内容”）
            3. wsserver_t::message_ptr msg = 消息对象（收到的短信内容）
                客户端发什么，这里就存什么
                你只用它：msg->get_payload() 拿字符串

        第三组：HTTP 相关（管 “网页请求”）
            4. websocketpp::http::parser::request req = HTTP 请求对象
                登录、注册、网页访问用
                拿 Cookie、路径、方法
            
            5. websocketpp::http::status_code::ok = HTTP 状态码（200）
                第四组：工具相关（定时器）

            6. wsserver_t::timer = 定时器
                匹配超时、房间过期用
    */

    //17.实现：“websocket回到函数——游戏房间的消息处理”
    void wsmsg_game_room(wsserver_t::connection_ptr &conn,wsserver_t::message_ptr &msg)
    {
        //1.（校验登录状态）通过智能连接指针获取会话对象->方便后面使用session类成员获取用户ID
        std::shared_ptr<session> ssp = get_session_from_cookie(conn);
        if(ssp==nullptr)
        {
            return;
        }

        //2.（校验房间存在性）通过用户ID获取其所属的房间对象->方便后面使用room类成员进行房间模块处理
        std::shared_ptr<room> rp = _room_user.get_room_by_rid(ssp->get_uid());
        if(rp==nullptr)
        {
            return;
            return ws_resp(conn,"unknow",false,"没有找到玩家的房间信息");
        }

        //3.提取消息对象的消息正文并将其进行反序列化
        Json::Value body_resp;
        std::string body = msg->get_payload();
        bool ret = json_util::unserialize(body,body_resp);
        if(ret==false)
        {
            return ws_resp(conn,"",false,"请求信息解析失败");
        }

        //4.交给房间模块处理
        rp->handle_request(body_resp);
    }

    //18.实现“websocket消息回调分发器”
    void wsmsg_callback(websocketpp::connection_hdl hdl,wsserver_t::message_ptr msg)
    {
        //1.通过连接句柄获取连接指针
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);

        //2.获取请求对象并提取url
        websocketpp::http::parser::request req = conn->get_request();
        std::string url = req.get_uri();
        
        //3.请求分发
        if(url=="/hall")
        {
            return wsmsg_game_hall(conn,msg);
        }
        else if(url=="/room")
        {
            return wsmsg_game_room(conn,msg);
        }
    }


    public:
    /*
        回想我们这个server.hpp文件中的唯一类 class gobang_server 的属性有哪些呢？
        它的属性主要是分成了三部分：
        
            //1.静态资源目录
            std::string _web_root;            ---> 直接指定为全局常量中的服务器配置 WWWROOT
            //2.websocket服务器对象
            wsserver_t _wssrv;                ---> 

            //3.用户表操作对象
            user_table _tb_user;              ---> 使用“初始化列表进行初始化” （主机+用户名+密码+数据库名+端口号）
            //4.在线管理对象
            online_manager _online_user;      ---> 使用“初始化列表进行初始化” (默认构造函数)
            //5.房间管理对象
            room_manager _room_user;          ---> 使用“初始化列表进行初始化” （用户表操作对象+在线管理对象）

            //6.会话管理对象
            session_manager _session_user;     ---> 使用“初始化列表进行初始化” （websocket服务器对象）
            //7.匹配管理对象
            matcher _matcher_user;             ---> 使用“初始化列表进行初始化”（用户表操作对象+在线管理对象+房间管理对象）
    */
    gobang_server(
        const std::string &host,
        const std::string &username,
        const std::string &password,
        const std::string &dbname,
        uint64_t port=3306,

        // user_table tb_user,
        // online_manager online_user,
        // room_manager room_user,

        // wsserver_t _wssrv,
        // std::string _web_root=WWWROOT
        const std::string wwwroot = WWWROOT
    ):_tb_user(host,username,password,dbname,port),
    _room_user(&_tb_user,&_online_user),  
    _matcher_user(&_tb_user,&_online_user,&_room_user),
    
    _session_user(&_wssrv)
    /*
        注意一下上面的细节：
           1. 首先通过原始5大组件创建出_tb_user对象
           2. 然后通过（提供组件创建出来的_tb_user对象 + 使用默认构造创建出来的_online_user对象）创建出_room_user
           3. 最后就是通过已经获取的三个对象创造出最后的匹配对象_matcher_user

           4. 最后通过websocket服务器对象，创建出_online_user
    */
    {
        /*  
            1. 为什么构造函数里没看到 _wssrv(...)？
                因为 websocketpp 的服务器对象，定义出来就已经初始化好了！
                你在类里一定是这么写的：
                // 成员变量声明
                wsserver_t _wssrv;   
                只要写了这一行，对象就已经创建、初始化完成了！
                它不需要像 _sm、_rm 那样在构造函数里再初始化。

            2. 那构造函数里这一堆操作是干嘛的？
               这不是 “创建对象”，而是 “给服务器做配置”！
               就像：
                    手机买好了（已经初始化）
                    现在要设置音量、网络、铃声
                    构造函数里做的就是：服务器配置 + 绑定回调

            3. 整个构造函数的终极作用
               一句话总结：初始化服务器所有模块 → 配置 websocket 服务器 → 绑定所有回调函数
               让服务器知道：谁来处理请求、谁来处理连接、谁来处理消息！

        */


        /* 所有服务器的构造函数，永远只干这 4 件事：
            gobang_server(...): 初始化所有模块
            {
                1. 关闭日志
                2. 初始化网络
                3. 设置端口复用
                4. 绑定所有回调函数
            }
        */

        //1.做第一件事情就是：“关闭日志”
        _wssrv.set_access_channels(websocketpp::log::alevel::none);

        //2.做第二件事情就是：“初始化ASIO”
        _wssrv.init_asio();

        //3.做第三件事情就是：“设置端口复用”
        _wssrv.set_reuse_addr(true);

        //4.绑定所有回调函数
        /*
            现在情况：
               1. 回调函数
               2. 回调分发函数
               3. 服务器绑定回调分发函数
        */
        _wssrv.set_http_handler(std::bind(&gobang_server::http_callback,this,std::placeholders::_1));
        //我不是很明白就是像是set_http_handler函数的内部就是写一个bind就行了嘛
        //bind函数的内部成员都写什么呢
        //我记的之前使用bind函数都是写成bind(&成员函数名称，调用函数的对象)
        //现在和之前不一样了，这是怎么回事

        /*
            set_http_handler 是 websocketpp 库自带的函数！
            作用：告诉服务器 —— 收到 HTTP 请求时，调用哪个函数处理！

            2.它到底是什么？
               你可以把它理解成：“注册一个处理 HTTP 请求的函数”
               当浏览器访问你的网页：http://127.0.0.1:8080/index.html
               websocketpp 服务器收到后，不知道该怎么办于是它去找你注册的回调函数：set_http_handler(谁来处理http请求)

            3. 最关键：怎么用？（固定万能写法）
                固定格式，背会它，所有项目都能用！
                // 格式
                set_http_handler(
                    std::bind(
                        &你的类名::处理函数,
                        this,
                        std::placeholders::_1
                    )
                );

            4. 这个函数的参数是什么？
                set_http_handler 需要一个可调用对象（函数 / 函数对象）这个函数的格式必须是：
                void 函数名(wsserver_t::connection_ptr conn);
                所以你必须写一个成员函数来接收：
                void http_callback(wsserver_t::connection_ptr conn)
                {
                    // 在这里处理 HTTP 请求
                    // 比如返回登录页面、处理注册接口
                }

            5. 为什么要用 std::bind？
                因为：http_callback 是类的成员函数，不能直接传给库！
                成员函数 = 需要对象（this）才能调用所以必须用 std::bind 把：
                    函数地址
                    this 指针
                    参数
                打包成一个库能识别的回调函数！          
        */


        /*
            bind 内部的参数是固定格式，所有服务器都这么写！
                1. 你以前学的 bind 是这样： bind(&类名::函数, 对象指针);
                   bind(&gobang_server::test, this); 这是无参函数。
                
                2. 现在 websocketpp 的回调 是带参数的！比如：
                   void http_callback(wsserver_t::connection_ptr conn) 它有 1 个参数。
                   void wsmsg_callback(wsserver_t::connection_ptr conn, wsserver_t::message_ptr msg) 它有 2 个参数。

                3. 带参数的成员函数，bind 必须这样写！
                   固定公式：bind(&类名::成员函数, this, 占位符1, 占位符2...)
                   占位符是什么？就是 “将来调用时传进来的参数”！
                   _1 = 第一个参数
                   _2 = 第二个参数

                4. 我给你逐行翻译，你马上秒懂！
                    ① HTTP 回调（1 个参数）
                        // 函数原型
                        void http_callback(connection_ptr conn);

                        // bind写法
                        bind(&gobang_server::http_callback, this, std::placeholders::_1)
                        含义：调用 this->http_callback
                        将来调用时，把库传进来的参数 放到 _1 的位置

                    ② 连接打开（1 个参数）
                        void wsopen_callback(connection_ptr conn);
                        bind(&gobang_server::wsopen_callback, this, _1)

                    ③ 连接关闭（1 个参数）
                        void wsclose_callback(connection_ptr conn);
                        bind(&gobang_server::wsclose_callback, this, _1)
            
                    ④ 消息回调（2 个参数！）
                        void wsmsg_callback(connection_ptr conn, message_ptr msg);
                        bind(..., this, _1, _2) 这里必须写两个占位符！因为函数有 两个参数！
        */
        _wssrv.set_open_handler(std::bind(&gobang_server::wsopen_callback,this,std::placeholders::_1));
        _wssrv.set_close_handler(std::bind(&gobang_server::wsclose_callback,this,std::placeholders::_1));
        _wssrv.set_message_handler(std::bind(&gobang_server::wsmsg_callback,this,std::placeholders::_1,std::placeholders::_2));

        /*
            它明确告诉你：open_handler 要求的回调签名是：
            回调函数(websocketpp::connection_hdl); 但你 bind 进去的函数，参数类型不对。

            1. 回调函数的参数写错了类型
                比如你写的是：
                    // ❌ 错误示例：用了 connection_ptr 而不是 connection_hdl
                    void wsopen_callback(wsserver_t::connection_ptr conn);

            2. 回调函数的参数带了引用（&）
                比如你写的是：
                    // ❌ 错误示例：多了 &
                    void wsopen_callback(websocketpp::connection_hdl &hdl);
                但 open_handler 期望的是：
                    // ✅ 正确签名：按值接收 connection_hdl
                    void wsopen_callback(websocketpp::connection_hdl hdl);
                    编译器会把 &hdl 当成 std::weak_ptr<void> &，导致类型不匹配。


            我用最通俗、最本质、最透彻的方式告诉你！
            因为 websocketpp 库规定死了：回调函数参数必须是值传递，不能是引用！
            你写 & 就类型不匹配，编译器直接报错！

            为什么库要这么设计？
            connection_hdl 本质 = 一个句柄 / 编号
            它内部就是个指针 / 编号，极小、极轻量
            复制成本几乎为 0，完全没必要用引用传递，所以库设计成值传递最安全、最简单
        */
    }


    //20.实现：‘启动服务器’
    void start(uint64_t port)
    {
        //1.监听指定端口
        _wssrv.listen(port);

        //2.开始接受连接
        _wssrv.start_accept();

        //3.进行ASIO事件循环
        _wssrv.run();
    }
    /*
        你的代码：listen() → start_accept() → run() 完全正确！
        这就是 websocketpp / ASIO 服务器的【标准启动 3 步曲】！99% 的项目都是这 3 行，没有多余步骤！
        这三行 = 服务器正式启动、开始工作

        二、你记忆里的流程：
        打开 → 绑定 → 监听 → 接受 → 处理这个是 原生 socket 底层流程！
        我给你 一一对应，你马上就懂！
        原生 socket 流程（底层）
            socket() 创建套接字 → 已经被 websocketpp 封装了
            bind() 绑定端口 → 封装进 listen () 里了
            listen() 监听端口 → 你写了
            accept() 接受连接 → 你写了 start_accept ()
            recv/send() 处理数据 → run () 里自动处理

        三、为什么你的代码只有 3 步？
            因为 websocketpp 把底层复杂的操作全部封装了！
            你不需要手动写：
                socket()
                bind()
                fcntl()
                setsockopt()
            这些底层操作 库全部帮你做了
            你只需要写 最上层的 3 个动作 即可！


        四、我给你逐行解释这 3 步到底干了什么
            1. listen(port)
                作用：告诉操作系统 —— 我要占用这个端口！
                    绑定端口，开始监听，底层做了：socket()，bind()，listen()
            
            2. start_accept()
                作用：开始等待客户端连接进来

            3. run()
                作用：启动死循环，永久运行服务器！这是一个阻塞函数不调用这个，服务器不会处理任何连接！ 
                它内部无限循环：
                    有没有新客户端连接？
                    有没有消息发来？
                    有没有断开事件？
                    调用对应的回调函数
    */
};

#endif


// wsserver_t == 基于asio_no_tls配置的WebSocket服务器类
// typedef websocketpp::server<websocketpp::config::asio> wsserver_t;
/*
    我给你翻译成人话：
        websocketpp：一个 WebSocket 网络库
        server：服务器
        asio：底层用的高性能网络框架
    合起来：基于 ASIO 异步模型的 WebSocket 服务器类型
*/
