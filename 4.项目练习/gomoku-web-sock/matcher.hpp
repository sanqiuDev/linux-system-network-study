#ifndef MATCHER_HPP
#define MATCHER_HPP

#include "util.hpp"

#include "db.hpp"
#include "online.hpp"
#include "room.hpp"

//1.首先来实现：“通用匹配队列模板类”
template <typename T>
class matcher_queue
{
    private:
    /*
        为什么属性 正好是这 3 个？
            1. 因为这 3 个就是 “线程安全队列” 的全部器官！
            2. 少一个都活不下去！
            3. 我一个一个给你讲为什么必须存在：

    1. std::list<T> _list; 真正存数据的容器（身体）：没有它 → 没地方存数据
    2. std::mutex _mutex; 作用：线程安全锁（安全锁）：没有它 → 多线程环境直接报废
    3. std::condition_variable _cond; 作用：阻塞等待（睡觉闹钟）：没有它 → CPU 100% 占用，服务器卡死
    
    为什么是这 3 个属性？
        一个存数据
        一个保证线程安全
        一个实现阻塞等待
    */

    //1.链表
    std::list<T> _list;

    //2.互斥锁
    std::mutex _mutex;

    //3.条件变量
    std::condition_variable _cond;
    
    public:
    //1.实现：“获取队列中元素的数量”
    int size()
    {
        //1.加锁
        std::unique_lock<std::mutex> lock(_mutex);
        //2.直接返回队列中的数量
        return _list.size();
    }
    //2.实现：“判断队列是否为空”
    bool empty()
    {
        //1.加锁
        std::unique_lock<std::mutex> lock(_mutex);

        //2.直接判断队列是否为空
        return _list.empty();
    }

    //3.实现：“阻塞当前线程，等待条件变量被唤醒”
    void wait()
    {
        //1.加锁
        std::unique_lock<std::mutex> lock(_mutex);

        //2.阻塞当前线程
        _cond.wait(lock);
        /*
           先给你一句终极人话总结，这个 wait () 就是：
               1. “没人跟我匹配 → 我先睡觉，不占 CPU；
               2. 有人来了 → 叫醒我，我再继续干活！”


          这一行是整个函数的灵魂！它做了 3 件神奇的事：
            ① 自动解锁：把刚才加的锁释放掉！→ 让别的线程可以继续入队、出队。
            ② 阻塞当前线程（睡觉）：线程停在这里，不再往下执行代码。→ 不占 CPU！不浪费资源！
            ③ 被唤醒后，自动重新加锁：当别人调用 notify_one() 叫醒它时：它会重新把锁锁上，然后从 wait 处继续往下执行
        */
    }

    //4.实现：“入队数据，并唤醒所有阻塞的匹配线程”
    void push(const T&data)
    {
        //1.加锁
        std::unique_lock<std::mutex> lock(_mutex);

        //2.将数据添加到链表的尾部
        _list.push_back(data);

        //3.唤醒所有的阻塞的匹配的线程
        _cond.notify_all();
    }

    //5.实现：“出队数据，并从队首取出数据”
    bool pop(T&data)
    {
        //1.加锁
        std::unique_lock<std::mutex> _mutex;

        //2.判断队列是否为空
        if(_list.empty()==true)
        {
            return false;
        }

        //3.取出队首元素
        data=_list.front();

        //4.出队数据
        _list.pop_front();
        retun true;
    }

    //6.实现：“移除队列中指定的数据”
    void remove(T &data)
    {
        //1.加锁
        std::unique_lock<std::mutex> _mutex;

        //2.直接删除
        _list.remove(data);
        /*
            1. remove (val) → 传【值】
                直接传数据本身
                删除所有等于这个值的元素
                专门给你这种场景用的

            2. erase (it) → 传【迭代器】
                只能传迭代器，不能传值！
                你传 data（数字）进去 → 直接报错！
        */
    }

};

//接下来我们来是实现一个新类：“匹配器管理类”
class matcher
{
    private:
    //1.分层匹配队列
    matcher_queue<uint64_t> _q_normal; //普通玩家队列
    matcher_queue<uint64_t> _q_high;   //高手玩家队列
    matcher_queue<uint64_t> _q_super;  //大神玩家队列

    //2.匹配处理线程
    std::thread _th_normal;  //普通匹配线程
    std::thread _th_high;    //高手匹配线程
    std::thread _th_super;   //大神匹配线程

    //3.外部依赖指针 -> 注意添加对应的头文件
    user_table *_tb_user;         //用户管理指针
    online_manager *_online_user; //在线管理指针
    room_manager *_room_user;     //房间管理指针

    //1.实现：“通用匹配处理函数”
    void handle_match(matcher_queue<uint64_t> &mq) //这里应该不难想到就是：处理匹配的话，我们传参需要传入一个匹配队列
    {
        //1.使用while死循环持续处理这个队列中的匹配请求
        while(1)
        {
            //1.1：判断当前匹配队列中玩家的数量是否>=2，不满足的话就进行阻塞等待
            if(mq.size()<2)
            {
                mq.wait();
            }

            //1.2：队列中玩家的数量足够的话，出队两名玩家
            uint64_t uid1,uid2;
            bool ret=mq.pop(uid1);
            if(ret==false)
            {
                // DLOG("出队玩家失败");
                //出队失败，跳过本次循环
                continue;
            }
            ret=mq.pop(uid2);
            if(ret==false)
            {
                // DLOG("出队玩家失败");
                //第二个玩家出队失败，将第一个玩家重新入队，并跳过本次循环
                this->add(uid1);
                continue;
            }

            //1.3：校验两个玩家是否在线
            /* 首先我们需要思考的问题就是什么才算是玩家在线呢？
                1. 我首先想到是判断玩家是否在游戏大厅中
                      在：在线
                      不在：离线
                   可以使用online文件中的接口：is_in_game_hall
            */
           /*
           if(online_user->is_in_game_hall(uid1)==false)
           {
              //注意细节：
              //玩家1离线，无法进行匹配对战，将玩家2重新入队，跳过本轮循环
              this->add(uid2);
              continue;
           }
           if(online_user->is_in_game_hall(uid2)==false)
           {
             //玩家2离线，无法进行匹配对战，将玩家1重新入队，并跳过本次循环
             this->add(uid1);
             continue;
           }
           */
          //提示：上面的代码不推荐，
          //我一句话先说原因：判断在线 → 必须看【连接是否存在】，不能只看状态！
          //重新规划思路：可以使用online.hpp文件中的接口get_conn_from_hall

          wsserver_t::connection_ptr conn1=_online_user->get_conn_from_hall(uid1);
          if(conn1==nullptr) //记住是和nullptr进行比较不是和false进行比较
          {
            //玩家1离线，无法进行匹配对战，将玩家2重新入队，跳过本轮循环
            this->add(uid2);
            continue;
          }
          wsserver_t::connection_ptr conn2 = _online_user->get_conn_from_hall(uid2);
          if(conn2==nullptr)
          {
            //玩家2离线，无法进行匹配对战，将玩家1重新入队，并跳过本次循环
            this->add(uid1);
            continue;
          }


          //1.4：两个玩家均同时在线为其创建房间
          /*问题怎么为两名玩家创建房间？
              1. 首先是使用class room的构造函数创建一个房间
              2. 然后就是将两名玩家都加入到这个房间中
          */ 
          //哈哈，其实怎么创建房间这种问题在class room_manager中已经为我们解决
          std::shared_ptr<room> rp =_room_user->create_room(uid1,uid2);
          if(rp==nullptr) //记住是和nullptr进行比较不是和false进行比较
          {
            //房间创建失败,将这两个玩家都加入到匹配队列中，并跳过本轮循环
            this->add(uid1);
            this->add(uid2);
            continue;
          }


          //1.5：向两名玩家发送匹配成功的响应
          Json::Value json_resp;
          json_resp["optype"]="match_success";
          json_resp["result"]=true;

          //broadcast(json_resp);
          /*
               broadcast 是房间里的广播函数，你现在还在【匹配阶段】，房间还没创建！根本不能用 broadcast！
                     broadcast 是 房间类（room） 的函数
                     作用：给当前房间里的两个人发消息
                     但你现在还在匹配队列，房间还没创建出来！
                     所以你根本调不到房间的 broadcast！

                别人写的代码（正确 ✅）
                    匹配成功时只有玩家连接（conn1、conn2）
                    没有房间，没有房间对象
                    只能直接通过连接发消息
                    必须手动序列化 → 字符串 → send
                    这才是匹配阶段正确的发送方式！
          */
           //1）先进行序列化
           std::string  body;
           json_util::serialize(json_resp,body);

           //2.将JSON串发送给匹配成功玩家双方
           conn1->send(body);
           conn2->send(body);
           
        }
    }
    
    //2.实现：“普通队列处理线程入口函数”
    void th_normal_entry(){return handle_match(_q_normal);}

    //3.实现：“高手队列处理线程入口函数”
    void th_high_entry(){return handle_match(_q_high);}

    //4.实现：“大神队列处理线程入口函数”
    void th_super_entry(){return handle_match(_q_super);}




    public:
    //5.实现：“构造函数”
    /*
       首先回想这个matcher类中属性成员都有哪些？
           1.三种匹配队列 
           2.三种匹配线程
           3.三种依赖指针 ---> 需要初始化

        需要注意的就是上面的“三种匹配线程”也是需要进行初始化的
    */
   matcher(user_table *tb_user,online_manager *online_user,room_manager *room_user):
   _tb_user(tb_user),_online_user(online_user),_room_user(room_user),
   /*
        这行代码 = 创建一个线程 + 让线程去跑匹配函数 + 把线程绑定到成员变量 _th_normal
        我拆成 4 个部分，你马上就懂👇
            _th_normal 是什么？
                它是成员变量，类型是：std::thread _th_normal;
                作用：管理一个线程对象（就像你握着一根绳子，绳子那头拴着一个线程）

            std::thread(...) 是什么？
                创建线程！
                C++ 创建线程的固定写法就是：std::thread(线程要跑的函数, 谁去跑这个函数);

            &matcher::th_normal_entry 是什么？
                线程跑的函数！
                也就是：“线程启动后，去执行 th_normal_entry 这个匹配函数”
                这就是你们普通场匹配逻辑的入口。

            this 是什么？
                告诉线程：是当前这个 matcher 对象去跑！
                因为 th_normal_entry 是成员函数，必须指定是哪个对象来调用它。
        
        _th_normal(std::thread(&matcher::th_normal_entry, this))
        翻译成人话：创建一个线程，让它执行当前对象的普通场匹配函数，并把这个线程交给 _th_normal 管理。


        为什么要写在“构造函数初始化列表”里？因为：
            1. std::thread 被创建的一瞬间，线程就开始跑了
            2. 写在初始化列表 = 对象一构造，三个匹配线程直接启动
   */
   _th_normal(std::thread(&matcher::th_normal_entry,this)),
   // 线程对象 ( 调用线程的构造函数（线程要跑的函数地址，对象指针）)
   _th_high(std::thread(&matcher::th_high_entry,this)),
   _th_super(std::thread(&matcher::th_super_entry,this))
   {
      DLOG("游戏匹配模块初始化完毕……");
   }

    public:
    //5.实现：“构造函数”
    /*
       首先回想这个matcher类中属性成员都有哪些？
           1.三种匹配队列 
           2.三种匹配线程
           3.三种依赖指针 ---> 需要初始化

        需要注意的就是上面的“三种匹配线程”也是需要进行初始化的
    */
   matcher(user_table *tb_user,online_manager *online_user,room_manager *room_user):
   _tb_user(tb_user),_online_user(online_user),_room_user(room_user),
   /*
        这行代码 = 创建一个线程 + 让线程去跑匹配函数 + 把线程绑定到成员变量 _th_normal
        我拆成 4 个部分，你马上就懂👇
            _th_normal 是什么？
                它是成员变量，类型是：std::thread _th_normal;
                作用：管理一个线程对象（就像你握着一根绳子，绳子那头拴着一个线程）

            std::thread(...) 是什么？
                创建线程！
                C++ 创建线程的固定写法就是：std::thread(线程要跑的函数, 谁去跑这个函数);

            &matcher::th_normal_entry 是什么？
                线程跑的函数！
                也就是：“线程启动后，去执行 th_normal_entry 这个匹配函数”
                这就是你们普通场匹配逻辑的入口。

            this 是什么？
                告诉线程：是当前这个 matcher 对象去跑！
                因为 th_normal_entry 是成员函数，必须指定是哪个对象来调用它。
        
        _th_normal(std::thread(&matcher::th_normal_entry, this))
        翻译成人话：创建一个线程，让它执行当前对象的普通场匹配函数，并把这个线程交给 _th_normal 管理。


        为什么要写在“构造函数初始化列表”里？因为：
            1. std::thread 被创建的一瞬间，线程就开始跑了
            2. 写在初始化列表 = 对象一构造，三个匹配线程直接启动
   */
   _th_normal(std::thread(&matcher::th_normal_entry,this)),
   // 线程对象 ( 调用线程的构造函数（线程要跑的函数地址，对象指针）)
   _th_high(std::thread(&matcher::th_high_entry,this)),
   _th_super(std::thread(&matcher::th_super_entry,this))
   {
      DLOG("游戏匹配模块初始化完毕……");
   }

    //6.实现：“添加玩家到对应档次的匹配队列”
    //本质问题：将玩家添加到匹配队列上本质就是“玩家ID放入链表中”，分档次本质就是“按照玩家分数分支放入指定链表”
    bool add(uint64_t uid)
    {
        //1.获取玩家的详细信息
        Json::Value user; //先定义个Json::Value类型容器用来存储通过用户ID获取详细信息
        bool ret=_tb_user->select_by_id(uid,user);
        if(ret==false)
        {
            DLOG("获取玩家：%lu的详细信息失败",uid);
            return false;
        }
        uint64_t score=user["score"].asUInt64();
        
        //2.按照玩家的天梯分数添加进不同匹配队列中
        if(score<2000)
        {
            _q_normal.push(uid);
        }
        else if(score>=2000&&score<3000)
        {
            _q_high.push(uid);
        }
        else
        {
            _q_super.push(uid);
        }
        
        return true;
    }

    //7.实现：“从对应的匹配队列种移除玩家”
    bool del(uint64_t uid)
    {
        //1.获取玩家的详细信息
        Json::Value user; //先定义个Json::Value类型容器用来存储通过用户ID获取详细信息
        bool ret=_tb_user->select_by_id(uid,user);
        if(ret==false)
        {
            DLOG("获取玩家：%lu的详细信息失败",uid);
            return false;
        }
        uint64_t score=user["score"].asUInt64();
        
        //2.按照玩家的天梯分数移除进不同匹配队列中
        if(score<2000)
        {
            _q_normal.remove(uid);
        }
        else if(score>=2000&&score<3000)
        {
            _q_high.remove(uid);
        }
        else
        {
            _q_super.remove(uid);
        }
        
        return true;
    }


};
#endif