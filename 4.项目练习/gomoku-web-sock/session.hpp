#ifndef SESSION_HPP
#define SESSION_HPP

#include "logger.hpp"

#include <cstdint>
#include <websocketpp/server.hpp>             // 引入WebSocket++服务器核心头文件（提供WebSocket服务器、定时器等核心功能）
#include <websocketpp/config/asio_no_tls.hpp> // 引入WebSocket++的ASIO无TLS配置头文件（基于ASIO的非加密通信配置）

#include <unordered_map>


//1.首先我们需要一些会话状态枚举类型
typedef enum
{
    UNLOGIN,
    LOGIN
} ss_statu;
/*
    为什么要怎么做？
    给会话定义两种状态：
        刚连上来 → UNLOGIN（未登录）
        登录成功 → LOGIN（已登录）
    为什么要写枚举？
    因为状态只有两种，用枚举最清晰、最安全。
*/
typedef websocketpp::server<websocketpp::config::asio> wsserver_t;


//2.接下来实现第一个类：会话类class session
class session
{
    private:
        //首先要明确用会话类的属性是什么：
        /*
           客户端和服务器之间的一次 “在线连接”
           只要客户端连着服务器，就有一个 session，它必须记录 4 件事：
                1. 我是谁？（唯一 ID）
                2. 我绑定了哪个用户？（用户 ID）
                3. 我登录了没？（状态）
                4.我多久超时销毁？（定时器）
        */
        uint64_t _ssid; //注：想要使用uint64_t 这个类型需要包含<cstdint>，但是像<string>等头文件间接包含了<cstdint>
        uint64_t _uid;
        ss_statu _statu;
        wsserver_t::timer_ptr _tp;   // 会话关联的定时器智能指针（用于控制会话自动销毁）
        /*  
            typedef websocketpp::server<websocketpp::config::asio> wsserver_t;
            我之前见过，类名::静态函数();，这种的
            这是调用静态成员函数。但 :: 还有第二种超级常用的用法：访问类内部定义的类型（类型别名/嵌套类型）
            类名::类型名

        在 websocket++ 里面，代码大概是这样写的：
        namespace websocketpp 
        {
            // 这是一个内部类！你看不到！
            class timer { ... };

            class wsserver_t 
            {
                public:
                    // 库作者给你起一个别名，让你用
                    using timer_ptr = std::shared_ptr<timer>;
            };
        }
            wsserver_t 是一个类，timer_ptr 是这个类里面定义的一个别名
            它本质 = std::shared_ptr<定时器>

            所以你写：wsserver_t::timer_ptr _tp;
            真正的意思是：std::shared_ptr<timer> _tp;

            这个是时候可能会好奇就是，啊就是共享智能指针，那我为什么不直接写成：
            原因就是这个这个智能指针管理对象的类型是timer，是websocket++的一个内部类
            话的定时操作，完全就是由 websocketpp 库底层自带的定时器功能实现的！
            ✅ 你根本不用自己写线程、不用自己写 sleep、不用自己造轮子！

            websocketpp 底层基于 asio 实现的定时器（asio 就是你刚才看到的 <websocketpp::config::asio>）
             定时器的底层运行、线程管理、时间精度、事件循环全部由 websocketpp + asio 帮你做完了！**
            你只是使用它提供的定时器接口而已！

        */

    public:
    //1.实现构造函数
    session(uint64_t ssid):_ssid(ssid) //注意：4个参数中我们只初始化了ssid这个成员，会话唯一标识ID
    {
        DLOG("会话对象构建成功");
    }
    
    //2.实现析构函数
    ~session()
    {
        DLOG("会话对象析构成功");
    }

    //3.实现：“获取会话ID”的操作” --->获取用get，return就好，明确是会话ID
    uint64_t get_ssid(){return _ssid;}
    //5.实现：“设置会话状态” ---> 设置用set，用户return，明确是会话状态
    void set_statu(ss_statu statu){_statu=statu;}

    //6.实现：“获取会话绑定的用户ID” ---> 获取用get，return就好，明确是用户ID
    uint64_t get_uid(){return _uid;}
    //4.实现：“绑定会话到指定用户ID” ---> 绑定用set，不使用return，用户ID
    void set_uid(uint64_t uid){_uid =uid;}

    //7.实现：“获取会话关联的定时器”
    wsserver_t::timer_ptr get_timer(){return _tp;}
    //8.实现：“设置会话关联的定时器”
    void set_timer(wsserver_t::timer_ptr tp){_tp=tp;}

    //9.实现：“判断用户是否已登录”
    bool is_login(){return (_statu==LOGIN);}
};


//------------------------------------------------------------------------------------
//这里我们需要一些配置信息
//1.会话默认的超时时间
#define SESSION_TIMEOUT 30000  //30s
//2.会话永久存在标记
#define SESSION_FOREVER -1
//3.简化session类的共享智能指针声明
// typedef std::shared_ptr<session>  session_ptr;

//接下来我们继续实现这个文件的第二个类：会话管理类：class session_manager
class session_manager
{
    private:
    /*
        为什么写「会话管理器」，一上来就要定义这 4 个变量？
         会话管理器 = 服务器的「总客服台」
         这 4 个变量 = 客服台必须有的 4 件装备！少一个都没法工作！
        会话管理器的使命：管理所有客户端连接（增、删、查、超时销毁）为了完成这个使命，它必须有这 4 样东西：
            1. uint64_t _next_ssid; 
                作用：生成唯一的会话 ID（身份证发放器）
                没有它，你无法创建新会话！

            2. std::mutex _mutex;
                作用：多线程安全锁（保安）
                没有它，服务器多线程一跑就炸！
            
            3. std::unordered_map<uint64_t, session_ptr> _session;
                作用：保存所有会话（花名册 / 哈希表）
                没有它，你根本存不住所有会话！

            4. wsserver_t* _server;
                作用：WebSocket 服务器指针（创建定时器用）
                没有它，你无法设置会话超时！


    
        最后这4个参数中第3个参数中会话映射表中的“键 -> 值” 是“_ssid -> session对象”
        但是它的这个对象是用智能指针进行管理着的
         因为会话对象会在【很多地方同时被使用】谁都不能随便把它删了，
         所以必须用 shared_ptr（共享智能指针）来托管生命周期！

         如果不用 shared_ptr你用普通指针：会发生什么？
         有人把它 delete 了，别人还在使用，直接悬空指针 → 程序崩溃！
        所以必须用 shared_ptr
        规则：大家一起用，最后一个人不用了，才自动释放！
    */

    //1.定义下一个要分配的会话ID
    uint64_t _next_ssid;
    //2.定义全局互斥锁
    std::mutex _mutex;
    //3.定义会话映射表：（会话ID -> 会话智能指针）定义这个东西主要是管理所有有效的会话
    std::unordered_map<uint64_t,std::shared_ptr<session>> _session;  //注意这里要包含投文件<unoredered_map>

    //4.定义websocket服务器器指针，定义这个东西主要为了“创建/销毁”会话定时器
    wsserver_t* _server;
    /*
        注意这里的语法：wsserver* ---> 定义一个指针,这个指针指向一个 wsserver_t 类型的对象
    */

    public:
    //1.实现“构造函数”
    session_manager(wsserver_t* srv): _next_ssid(1),_server(srv)
    {
        DLOG("会话管理器创建成功");
    }

    //2.实现“析构函数”
    ~session_manager()
    {
        DLOG("会话管理器析构成功");
    }

    
    //3.实现：“创建新会话”
    std::shared_ptr<session> create_session(uint64_t uid,ss_statu statu)
    /*
        创建会话的时候，必须立刻知道：这个连接「是谁」、「登没登」！
            所以参数必须传：uid (用户 ID) + statu (状态)
        因为会话一出生，就必须带着身份和状态！
    */
   {
     //1.加互斥锁保护会话表的并发插入操作
     std::unique_lock<std::mutex> lock(_mutex);

     //2.创建新会话对象
     std::shared_ptr<session> ssp(new session(_next_ssid));
     /*
          std::shared_ptr 管理的对象，必须是堆上的对象！必须用 new 出来的！
          shared_ptr<T>( new T(...) );    ✅ 正确（堆对象）
          shared_ptr<T>( stack_obj );     ❌ 错误（栈对象，会崩溃）

          智能指针的 () 里到底能放什么？只有 两种合法写法：
          写法 1：直接放 new 创建的堆对象
                std::shared_ptr<session> ssp( new session(_next_ssid) );
                这行代码其实做了 两件事：
                    1. 在堆上开辟内存
                    2. 调用 session 的构造函数，创建对象
          写法 2：放一个已经存在的堆指针
                session* p = new session(_next_ssid);
                std::shared_ptr<session> ssp(p);
     */

     //3.设置会话绑定的用户ID和会话状态
     ssp->set_uid(uid);
     ssp->set_statu(statu);

     //4.将新会话添加进入会话映射表
     _session.insert(std::make_pair(_next_ssid,ssp));

     //5.更新下一个会话ID
     _next_ssid++;

     //6.返回用智能指针管理的新会话对象
     return ssp;
   }


//实现：“将已经存在的会话对象添加到会话映射表当中”
void append_session(std::shared_ptr<session> ssp)
{
    //1.加锁,因为接下来我们要操作会话映射表了
    std::unique_lock<std::mutex> lock(_mutex);
    //2.添加到会话映射表中
    // _session.insert(std::make_pair(_next_ssid,ssp));
    // 上面这是新建的会话添加时这么写的，而已存在的添加是这么写的
    _session.insert(std::make_pair(ssp->get_ssid(),ssp));
}

//5.实现：“通过会话ID获取会话对象”
std::shared_ptr<session> get_session_by_ssid(uint64_t ssid)
{
    //1.加锁
    std::unique_lock<std::mutex> lock(_mutex);

    // //2.直接返回会话对象
    // return _session[ssid];
    /*
    return _session[ssid];它的行为：
        1. 如果 ssid 存在 → 返回正确的会话
        2. 如果 ssid 不存在 → 自动创建一个空的 session 放进 map 里！

       后果：查询一个不存在的 ID，map 里多了一个无效的空会话，内存泄漏，逻辑错乱，服务器越跑越慢
    */

    //2.查找会话ID对应的记录
    auto it = _session.find(ssid);
    
    //3.返回对应的会话对象
    if(it==_session.end())
    {
        std::shared_ptr<session>();// 这是智能指针对象的默认构造。它的作用只有一个：创建一个空的、不指向任何对象的 shared_ptr
    }
    return it->second;
}



//6.实现：“通过会话ID销毁会话对象”
void remove_session(uint64_t ssid)
{
    //1.加锁
    std::unique_lock<std::mutex> lock(_mutex);

    //2.直接移除即可
    _session.erase(ssid);
}

//7.实现：“设置会话过期时间”
void set_session_expire_time(uint64_t ssid,int ms) //参数就是要设置的会话ID，以及会话的过期时间
{
    //1.通过会话ID获取会话对象
    std::shared_ptr<session> ssp= get_session_by_ssid(ssid);
    if(ssp.get()==NULL)  //ssp.get() = 把智能指针里包着的 “原始裸指针” 取出来,当然这里写不写都可
    {
        return;
    } 

    //2.获取会话当前关联的定时器
    //首先是怎么获取定时器：会话对象可以调用get_timer函数获取该会话的定时器
    //其次我们要知道这个定时器的类型是：wsserver_t::timer_ptr
    wsserver_t::timer_ptr tp=ssp->get_timer();

    //3.场景一：会话无定时器 + 设置为永久存在 ---> 无需任何操作
    if(tp==nullptr&&ms == SESSION_FOREVER)
    {
        return ;
    }

    //4.场景二：会话无定时器 + 设置临时超时 ---> 设置临时状态
    else if(tp==nullptr&&ms!=SESSION_FOREVER)
    {
        //1.创建websocket定时器
        /*
            _server->set_timer(毫秒, 要执行的函数);
            意思就是：等 xx 毫秒后，自动帮我调用这个函数 → 销毁会话！
        
        它分成 2 大部分：
            第一部分：_server->set_timer(ms, ...)
            作用：创建一个定时器
            ms：多少毫秒后执行
            第二个参数：时间到了，要执行什么动作
            这就是 websocketpp 库提供的定时器功能，你直接用就行！

            第二部分（你最晕的）：
            std::bind(...) 到底是什么？
            我用人话讲：std::bind = 把「函数 + 参数」打包成一个可执行的包裹
            定时器到时间，就解开包裹，执行函数！
            我再拆开 std::bind 里面的东西
                std::bind(
                    &session_manager::remove_session,  //1.要执行的函数（销毁会话）
                    this,  //2.用哪个对象去执行，因为 remove_session 是成员函数调用成员函数必须有对象，
                           //这里 this 就是当前会话管理器对象
                    ssid   //3.要传给函数的参数：会话ID，要传给 remove_session(uint64_t ssid) 的参数
                )
            打包了一句话：“xx 毫秒后，请调用 this->remove_session (ssid)”
        */

        //创建websocket定时器我们用的就是：wsserver_t类型的成员函数set_timer
        wsserver_t::timer_ptr tmp_tp=_server->set_timer(ms,std::bind(&session_manager::remove_session,this,ssid));
        /*注意：这里必须写成&session_manager::remove_session不能写成&remove_session
           因为它是写在 session_manager 类里面的,取类的成员函数地址，必须带 类名::
           终极死规矩（背下来）
                全局函数 → &函数名
                类成员函数 → &类名::函数名 ✅
        */

        //2.将定时器绑定到会话
        ssp->set_timer(tmp_tp);
    }

    //5.场景三：会话有定时器 + 设置为永久存在
    else if(tp!=nullptr&&ms==SESSION_FOREVER) //意思：这个会话本来会超时自动销毁,现在我要让它变成永久在线，不销毁
    {
        //1.取消当前定时器
        tp->cancel(); //但副作用：直接触发 remove_session (ssid)，把会话从 map 里删掉了！


        //2.清空与会话关联的定时器
        ssp->set_timer(wsserver_t::timer_ptr()); //作用：告诉会话：以后没有定时器了
        //注意细节：类名 () → 就是一个【临时的、空的】对象！

        /*
            websocketpp 定时器调用 cancel () 取消时，会直接触发回调函数！
            也就是：你本来想取消 “销毁会话”，结果一取消，反而立刻把会话删了！

            正常思路应该是：
                取消定时器
                清空定时器
                ✅ 结束
            但 websocketpp 坑就坑在：cancel () 会直接执行定时器的回调函数（remove_session）！你一取消，会话直接被删了！
        */


        //3.延迟 0 毫秒，把会话加回去
        /*这一步是最关键、最精髓、最绕的一步！因为：
            cancel() 已经把会话删了，但这个会话还活着（智能指针持有），所以必须重新加回 map
            为什么要延迟 0 毫秒？
            因为：cancel () 触发的 remove_session 是异步执行的必须等它执行完，我们再把会话加回去！
            0 毫秒 = 等当前事件完成 → 立刻执行
        */
        _server->set_timer(0,std::bind(&session_manager::append_session,this,ssp));
    }


    //6.场景四：“会话有定时器 + 重置临时超时” -> 更新销毁时间
    else if(tp!=nullptr&&ms!=SESSION_FOREVER)
    {
        //1.取消当前定时器
        tp->cancel();
        //2.情况当前会话关联的定时器
        ssp->set_timer(wsserver_t::timer_ptr());
        //3.延迟0毫秒重新添加会话
        _server->set_timer(0,std::bind(&session_manager::append_session,this,ssp));

        //4.创建新定时器
        wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms,std::bind(&session_manager::remove_session,this,ssid));

        //5.将定时器和会话进行绑定
        ssp->set_timer(tmp_tp);
    }
}

};



#endif