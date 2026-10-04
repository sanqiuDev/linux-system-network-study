#ifndef ROOM_HPP
#define ROOM_HPP

#include "util.hpp"
#include "db.hpp"
#include "online.hpp"

//首先要定义全局常量
#define BOARD_ROW 15     //1.五子棋的行数：board_row
#define BOARD_COL 15     //2.五子棋的列数：board_col
#define CHESS_WHITE 1    //3.白色五子棋：chess_white
#define CHESS_BLACK 2    //4.黑色五子棋：chess_black

//接着定义房间状态枚举类型
typedef enum
{
    GAME_START,   //房间状态一：游戏进行
    GAME_OVER     //房间状态二：游戏结束
} room_statu;

//好了接下来我们来实现第一个类吧就是：“五子棋房间类”：class room
class room
{
    private:
    /*
        room 房间类 = 一个真实的游戏房间，它必须知道：
            我是谁？（房间 ID）
            我现在什么状态？（开始 / 结束）

            里面有几个人？
            谁是黑棋？
            谁是白棋？
            棋盘长啥样？

            我要找谁帮我发消息？、改数据？（用户表、在线管理器）
    */
   //1.房间唯一 ID
   uint64_t _room_id;
   //2.房间状态：游戏中 / 游戏结束
   room_statu _statu;
   //3.当前房间的人数
   int _player_count;
   //4.白棋玩家的ID
   uint64_t _white_id;
   //5.黑棋玩家的ID
   uint64_t _black_id;
   //6.棋盘！
   std::vector<std::vector<int>> _board;

   //7.用户数据库指针
   user_table* _tb_user; //游戏结束后，要修改用户分数、胜负场,所以必须拿着 “用户表” 的指针
   //8.在线用户管理器指针
   online_manager* _online_user; //要给玩家发消息、广播下棋位置,必须通过它找到玩家的连接
   /*
      我包含了db.hpp头文件，online.hpp 头文件之后，难道我不能直接调用是修改用户分数，获取玩家连接得的函数嘛？
      非要定义这两个类的对象指针
      
      编译器会直接告诉你：非静态成员函数必须通过对象调用！
    user_table::add_score(uid); ❌ 错误写法
    这叫 **“类作用域”**它只是一个 “名字”，不是真实可调用的函数！
    “人类.吃饭()”：没有具体的人，怎么吃饭？



    还有一点就是我不是很明白就是为什么不写成：std::shared_ptr<online_manager>_online_user(new online_manager());这样呢？
    为什么不直接在房间里 new 一个智能指针对象？非要用裸指针指向外面？
    我直接给你一句话终极答案：因为整个游戏服务器，只需要【一个】用户管理器、【一个】数据库！
    你每个房间都 new 一个，数据就全乱了！
    这行代码的意思是：每创建一个房间，就创建一个全新的、独立的在线管理器！
    假如开了 5 个房间：
        房间 1 → 有自己的 online_manager
        房间 2 → 有自己的 online_manager
        房间 3 → 有自己的 online_manager
        ...
        结果：数据完全隔离，彻底乱套！
    房间只是 “使用者”，不是 “拥有者”

    那什么时候用你说的 shared_ptr？
    只有这个东西是房间 “自己独有的”，才用智能指针！
        比如：
            房间自己的棋盘
            房间自己的定时器
            房间自己的锁
            这些每个房间独立的东西，才需要自己 new。
   */

   //1.实现：“沿指定方向判断同色棋子是不是五子连珠了”
   bool five(int row,int col,int row_off,int col_off,int color)
   /*
      //1.首先需要当前落子的位置，行列坐标
      //2.其次需要偏移方向，行列偏移方向
      //3.最后是当前落子的棋子颜色
   */
    {
        //1.定义计数器
        int count=1;

        //2.沿偏移量正方向检测同色棋子
        int search_row=row+row_off;
        int search_col=col+col_off;
        while(search_row>=0&&search_row<BOARD_ROW&&
              search_col>=0&&search_col<BOARD_COL&&
              _board[search_row][search_col]==color
        )
        {
            //2.1：同色棋子的数量+1
            count++;

            //2.2：检索位置继续沿正方向偏移
            search_row+=row_off;
            search_col+=col+col_off;
        }

        //3.沿偏移量反方向检测同色棋子
        search_row=row-row_off;
        search_col=col-col_off;
        while(search_row>=0&&search_row<BOARD_ROW&&
              search_col>=0&&search_col<BOARD_ROW&&
              _board[search_row][search_col]==color
        )
        {
            //3.1：同色棋子的数量+1
            count++;

            //3.2：检索位置继续沿负方向偏移
            search_row-=row_off;
            search_col-=col_off;
        }

        //4.判断该指定方向上是否有5个棋子
        return count>=5;
    }

    //2.实现：“检查当前的落子是否导致玩家获胜”
    uint64_t check_win(int row,int col,int color)
    /*
       注意这里细节：我们返回的不死是否有玩家获胜，而是有玩家获胜了返回他执棋的颜色是什么
    */
   {
      //1.检测四个方向上是否存在五子连珠,有获胜者并返回其棋子颜色
      /* 四个方向的偏移量要怎么定义呢？
           1. 水平的偏移量：（0，1）
           2. 竖直的偏移量：（1，0）
           3. 正斜向（左上→右下) 的偏移量：（1，1）
           4. 反斜向（右上→左下）的偏移量：（1，-1）
      */
      if(five(row,col,0,1,color)||
         five(row,col,1,0,color)||
         five(row,col,1,1,color)||
         five(row,col,1,-1,color))
         {
            return color==CHESS_WHITE?_white_id:_black_id;
         }

         //2.无获胜者暂时返回0代表无获胜者
         return 0;
   }
   public:
   //1.实现构造函数
   room(uint64_t room_id,user_table* tb_user,online_manager* online_user):
   _room_id(room_id),_tb_user(tb_user),_online_user(online_user),
   _statu(GAME_START),_player_count(0),_board(BOARD_ROW,std::vector<int>(BOARD_COL, 0))
   /*
      参数传入：1）房间ID  2）用户数据库指针  3）在线用户管理器指针

      //这里简单介绍一下使用vector容器定义二维数组：
      std::vector<std::vector<int>> grid(3,std::vector<int>(4));
      std::vector<std::vector<int>> grid(行数,std::vector<int>(列数，初始值));
   */
   {
        DLOG("%lu，游戏房间对象初始化成功",_room_id);
   }

   //2.实现析构函数
   ~room()
   {
        DLOG("%lu，游戏房间对象析构成功",_room_id);
   }

   //3.获取房间ID
   uint64_t get_room_id(){return _room_id;}
   //4.获取房间状态
   room_statu get_room_statu(){return _statu;}
   //5.获取房间内玩家数量
   int get_player_count(){return _player_count;}
   //6.获取白棋玩家ID
   uint64_t get_white_id(){return _white_id;}
   //7.获取黑棋玩家ID
   uint64_t get_black_id(){return _black_id;}

   //8.添加白棋玩家
   void add_white_user(uint64_t uid){_white_id=uid,_player_count++;}
   //9.添加黑棋玩家
   void add_black_user(uint64_t uid){_black_id=uid,_player_count++;}


   //10.实现：“玩家下棋请求”
   Json::Value handle_chess(Json::Value &req)
   {
      //1.初始化响应对象
      Json::Value json_resp=req;

      //2.提取请求中的核心信息：1）落子的横坐标 2）落子的纵坐标  3）当前玩家ID
      int chess_row=req["row"].asInt();
      int chess_col=req["col"].asInt();
      uint64_t curr_user=req["user"].asUInt64();
      /*
        下棋请求协议（你定的！）
        客户端 → 服务器
         json
         {
            "uid": 玩家ID,
            "room_id": 房间号,
            "row": 落子行,
            "col": 落子列,
            "msg_type": 3   // 3表示“下棋消息”
         }

        服务器 → 客户端
         json
         {
            "result": 成功/失败,
            "winner": 赢家ID,
            "reason": "原因"
         }
      */

      //3.判断两个玩家是否都在线，任意一方掉线则另一方直接胜利并构建响应对象
      if(_online_user->is_in_game_room(_white_id)==false)
      {
         json_resp["result"]=true;
         json_resp["reson"]="对方掉线，不战而胜";
         json_resp["winner"]=(Json::UInt64)_black_id;  //注意：_black_id原本是uint64_t类型，现在需要转化为Json::UInt64类型

        return json_resp;
        /*
         绝对不可能直接返回 json_resp 给前端！
         前端绝对收不到 Json::Value 对象！网络绝对不能传 C++ 对象！
         return json_resp; 不是把这个对象发给前端！
         真实底层流程（必须记住）
         你写：return json_resp;
         框架 / 底层代码 会自动帮你做：std::string str = 序列化(json_resp);
         send(str);  // 发送字符串到网络
         前端收到的是：{"optype":"put_chess", ...}
        */
     }
      if(_online_user->is_in_game_room(_black_id)==false)
      {
         json_resp["result"]=true;
         json_resp["reson"]="对方掉线，不战而胜";
         json_resp["winner"]=(Json::UInt64)_white_id;  

         return json_resp;
      }

      //4.判断落子位置是否合法
      if(_board[chess_row][chess_col]!=0)
      {
        // DLOG("落子位置不合法");
        json_resp["result"] = false;
        json_resp["reason"] = "当前位置已经有了其他棋子！";
        return json_resp;
      }

      //5.记录落子信息更新棋盘数据WHITE?white:black;
      //5.1：判断当前玩家棋子的颜色
      int chess_color=curr_user==_white_id?CHESS_WHITE:CHESS_BLACK;   
      //注意细节：之前我们是知道玩家执棋的颜色获取玩家ID：color==CHESS_WHITE?_white_id:_black_id;
      //现在是知道玩家ID，获取执棋的颜色
    
      //5.2:更新棋盘
      _board[chess_row][chess_col]=chess_color;


      //6.判断当前落子是否会产生胜利,并返回前端相应
      uint64_t ret = check_win(chess_row,chess_col,chess_color);
      if(ret!=0)
      {
        DLOG("五星连珠");
      }
      json_resp["result"]=true;
      json_resp["winner"]=(Json::UInt64)ret;
      return json_resp;
   }

//11.实现：“处理玩家聊天的请求”
Json::Value handle_chat(Json::Value &req)
{
   //1.初始化响应对象
   Json::Value json_resp = req;

   //2.提取聊天信息，并进行敏感词的过滤
   /*
     请求（前端 → 服务器）
     {
         "uid": 1001,
         "room_id": 520,
         "msg_type": 4,
         "message": "你好！"   // 聊天内容
     }

    响应（服务器 → 前端）
    {
         "uid": 1001,
         "room_id": 520,
         "msg_type": 4,
         "message": "你好！",
         "result": true
    }   
   */
   std::string msg = req["message"].asString();
   auto pos =msg.find("垃圾");
   /*这里注意细节：
     auto pos = msg.find('垃');   // 返回迭代器
     if (pos != msg.end()) {      // ✅ 可以！
       // 找到了
     }

              写法	           返回值类型	  判断没找到用
    auto pos = str.find(...)	    迭代器	   pos != str.end()
    size_t pos = str.find(...)	数字下标	   pos != std::string::npos


    因为 C++ 的 string 有两套 find！
       1. 返回迭代器的 find（C++11 及以上）
       2. 返回下标数字的 find（最传统、最常用）
   */
   if(pos!=std::string::npos)
   {
      json_resp["result"]=false;
      json_resp["reson"]="消息中包含敏感词";
      return json_resp;
   }

   //3.聊天消息合法，返回成功响应
   json_resp["result"]=true;
   return json_resp;
}

//12.实现：“处理玩家退出房间的请求”
void handle_exit(uint64_t uid) //场景就是已经知道了用户ID了，将其移除游戏房间
{
   //1.初始化响应对象
   Json::Value json_resp;

   //2.判断游戏房间的状态是否是游戏进行时，如果是则判定对方胜利
   if(_statu==GAME_START)
   {
      //2.1：确定胜利的ID
      uint64_t winner=uid==_white_id?_black_id:_white_id;

      //2.2:构建退出响应消息
      /*
         退出函数的 JSON 响应字段都有什么？
           {
               "optype": "put_chess",      // 固定：下棋/退出都用这个让前端刷新界面
               "result": true,             // 操作成功
               "uid": 1002,                // 退出的玩家ID
               "room_id": 520,             // 房间号
               "winner": 1001,             // 赢家ID
               "reason": "对方掉线，不战而胜"
            }
      */
      json_resp["optype"]="put_chess";
      json_resp["reslut"]=true;
      json_resp["reason"]="对方掉线，不战而胜";
      json_resp["room_id"]=(Json::UInt64)_room_id;
      json_resp["uid"]=(Json::UInt64)uid;

      json_resp["row"]=-1;
      json_resp["col"]=-1;
      json_resp["winner"]=(Json::UInt64)winner;

      /*
         optype = 前端页面的 “遥控器”，它不是给人看的，是给前端代码看的！
         前端（网页 / 客户端）收到 JSON 后，第一时间看 optype 是什么，然后决定：
            1. 是刷新棋盘？
            2. 还是弹出聊天？
            3. 还是显示胜利画面？


         为什么退出要写 "put_chess"？
         因为：玩家退出 = 游戏直接结束 = 前端要显示 “下棋赢了” 的界面！
      你想想：
         正常下棋赢了 → 前端显示：你赢了！
         对方逃跑 / 退出 → 前端也应该显示：你赢了！
      所以服务器直接告诉前端：
         “当他退出处理，按下棋胜利来显示！”
         json_resp["optype"] = "put_chess"; 意思：告诉前端 —— 把这个当 “下棋结果” 来处理！
         翻译成人话：“玩家退出了，你直接当成下棋胜利来处理，弹出胜利界面！”
      */

      //2.3：更新数据库中玩家的对战结果
      //核心就是判断谁是胜利玩家谁失败玩家，然后调用user_table中分装好的数据更新到数据库中的操作即
      _tb_user->win(winner);
      _tb_user->lose(uid);

      //2.4：更新游戏房间状态设置为游戏结束
      _statu=GAME_OVER;

      //2.5：广播退出结果给另一个玩家
      broadcast(json_resp);
      //在 C++ 类里面，函数没有 “必须定义在前面才能调用” 的规则！
   }

   //3.减少游戏房间中玩家的数量
   _player_count--;
   return;
}

//13.统一处理房间中的所有的请求
void handle_request(Json::Value &req)
{
   //1.初始化响应对象
   Json::Value json_resp;
   /*
      为什么这里不写 Json::Value json_resp = req;，而是写 Json::Value json_resp;？
      答案：因为这里出错了，不需要把请求数据带回去！

      为什么不用 = req？
      因为：这是一个 “错误拦截” 逻辑！
         房间号都错了 → 请求本身就是无效的
         无效请求 → 不需要把 uid /row/col 这些数据返回给前端
      只需要告诉前端：你错了，原因是什么
      所以：只需要一个空的 json_resp，填错误信息就行！
   */

   //2.校验房间号是否匹配
   uint64_t room_id=req["_room_id"].asUInt64();
   if(room_id!=_room_id)
   {
      /*
         为什么这里必须写？因为：Json::Value json_resp;
         你创建的是空对象！空对象里 什么都没有！
         前端收到后一看：没有 optype → 不知道这是什么消息 → 直接不处理！
         所以必须手动加一句：json_resp["optype"] = req["optype"].asString();
         把原来的操作类型复制回去！
         告诉前端：你刚才发的是 “下棋” 请求，虽然失败了，但我还是告诉你这是下棋的返回结果。
      */
      json_resp["optype"]=req["optype"].asString();//注意细节：右边必须放字符串，不能放 Json::Value 对象！
      json_resp["result"] =false;
      json_resp["reson"]="房间号不匹配";
      return broadcast(json_resp);

      /*
           return json_resp = 只把结果返回给【当前发送请求的玩家】
                作用：服务器 只把消息发给当前下棋的这个人。
           return broadcast (json_resp) = 把结果【广播给房间里所有玩家】
                作用：服务器 把消息发给房间里的 2 个人！
      */
   }


   //3.根据请求类型分发到具体处理方法
   std::string optype=req["optype"].asString();
   //3.1：情况一：处理下棋请求
   if(optype=="put_chess")
   {
      json_resp=handle_chess(req);
      //这里需要注意的就是：handle_chess 函数的内部没有更新玩家的游戏数据，只是返回了获胜玩家的ID，所以这里我们要更新一下
      if(json_resp["winner"].asUInt64()!=0)
      {
         //1.先判定胜利玩家和失败玩家的ID
         uint64_t winner_id=json_resp["winner"].asUInt64();
         uint64_t loser_id=winner_id==_white_id?_black_id:_white_id;

         //2.更新玩家的数据库的信息
         _tb_user->win(winner_id);
         _tb_user->lose(loser_id);

         //3.设置游戏房间状态为结束
         _statu=GAME_OVER;
      } 
   }

   //3.2：情况二：处理聊天请求
   else if(optype=="chat")
   {
      json_resp=handle_chat(req);
   }

   //3.3：情况三：处理未知请求类型
   else
   {
      json_resp["optype"]=optype;
      /*
         optype 是提前定义好的字符串变量！
         它的类型是：std::string optype;  // 字符串！
      */
      json_resp["result"]="false";
      json_resp["reson"]="未知请求类型";
   }

   //4.序列化响应结果打印调试日志
   std::string body;
   json_util::serialize(json_resp,body);
   DLOG("%s",body.c_str());
   /*
      为什么打印日志也要序列化？因为：
         日志是人看的 → 人只能看懂字符串
         网络传输也是字符串
      所以序列化一次，两用：
         1. 打印日志给你看
         2. 发送网络给前端用
   */

   //5.广播响应结果给房间中的所有的玩家
   return broadcast(json_resp);
}


//14.实现：“将响应消息广播给房间内的两位玩家”
void broadcast(Json::Value &rsp)
{
  //1.将Json::Value格式的响应消息序列化为JSON格式的字符串
  std::string body;
  json_util::serialize(rsp,body); 

  //2.获取白棋玩家的websocker连接发消息
  /*
      一句话核心总结：connection_ptr = 客户端与服务器之间的 “电话线”
      它代表一个客户端的连接，你拿着这个指针，就能对这个客户端进行操作。

      它最核心、最常用的 4 个功能（你必须记住）
         1. 给这个客户端发送消息（最常用！）
              conn->send(msg_string);这就是给前端发数据的底层接口！
              你之前的 broadcast 广播，底层就是遍历所有连接，然后调用：conn->send(body);
         2. 判断客户端是否还在线（存活）：
               if (conn->is_valid()) {
                  // 客户端还连着
               }
               用来判断玩家有没有掉线。
         3. 主动断开客户端连接：
               conn->close(); 比如玩家违规、房间解散，直接踢人下线。
         4. 获取客户端的 IP 地址：
               std::string ip = conn->remote_endpoint_address();
               用来记录日志、判断玩家来源。
  */
   wsserver_t::connection_ptr wconn=_online_user->get_conn_from_room(_white_id);
   if(wconn!=nullptr) wconn->send(body);
   else DLOG("游戏房间中白棋玩家获取失败")

   //3.实现：“获取黑棋玩家的websocket连接并发送消息”
   wsserver_t::connection_ptr bconn=_online_user->get_conn_from_room(_black_id);
   if(bconn!=nullptr) bconn->send(body);
   else DLOG("游戏房间中黑棋玩家获取失败");

   return;
}

};




//接着我们来实现：“房间管理类：class room_manager”
class room_manager
{
   private:
   /*
      1. 生成唯一房间号：          没有它 → 房间号会重复 → 游戏大乱
      2. 多线程安全锁：            没有它 → 程序直接崩溃
      3. 数据库操作：              没有它 → 打完游戏战绩不更新
      4. 找到玩家的连接：          没有它 → 无法广播、无法聊天、无法下棋
      5. 管理所有房间：            没有它 → 找不到房间
      6. 快速知道 “玩家在哪个房间”：没有它 → 玩家发消息，服务器找不到他在哪个房间
   */
   //1.下一个要分配的房间ID
   uint64_t _next_rid; //还是一样的，之前在session.hpp文件中session和session_manager中就是：会话ID 和 下一个会话ID

   //2.一把互斥锁
   std::mutex _mutex;

   //3.用户管理指针
   user_table *_tb_user;
   //4.在线管理指针
   online_manager *_online_user;

   //5.房间ID -> 房间对象 的房间映射表
   std::unordered_map<uint64_t,std::shared_ptr<room>> _rooms;
   //6.用户ID -> 房间ID   的用户映射表
   std::unordered_map<uint64_t,uint64_t> _users;

   public:
   //1.实现：“构造函数”
   /*浅浅的谈一谈自己对构造函数的认识：
      1. 首先是互斥锁是在构造函数中是不进行初始化的
      2. 其次就是像一些映射表也不初始化

      3. 一些普通指针什么的要考虑进行初始化
   */
   room_manager(user_table *tb_user,online_manager *online_user):_next_rid(1),
   _tb_user(tb_user),_online_user(online_user)
   {
      DLOG("房间管理对象创建成功");
   }


   //2.实现：“析构函数”
   ~room_manager()
   {
      DLOG("房间管理对象析构成功");
   }

   //3.实现：“为匹配成功的两个玩家创建新房间”
   /*
      这里要能想明白一个问题就是：何为创建一个新的房间呢？
          1. 本质就是调用room类的构造函数（1.房间ID，2.用户管理指针，3.在线管理指针）
          2. 建立用户ID -> 房间ID的映射，并将其存入用户映射表当中
          3. 建立房间ID -> 房间对象的映射，并将其存入房间映射表当中

      所以我们可以推测出：
          1. 输入参数：1）玩家1的ID 2）玩家2的ID
          2. 返回值：就是新创建房间的对象
   */
   std::shared_ptr<room> create_room(uint64_t uid1,uint64_t uid2)
   {
      //1.校验两名玩家是否都是在游戏大厅当中
      if(_online_user->is_in_game_room(uid1)==false)
      {
         DLOG("用户%lu,不在游戏房间中",uid1);
         return std::shared_ptr<room>();
      }
      if(_online_user->is_in_game_room(uid2)==false)
      {
         DLOG("用户%lu,不在游戏房间中",uid2);
         return std::shared_ptr<room>();
      }

      //2.加互斥锁
      std::unique_lock<std::mutex> lock(_mutex);

      //3.创建游戏房间，添加两名玩家进入该游戏房间
      std::shared_ptr<room> rp(new room(_next_rid,_tb_user,_online_user));
      rp->add_white_user(uid1);
      rp->add_white_user(uid2);

      //4.建立用户ID -> 房间ID的映射，并将其存入用户映射表当中
      _users.insert(std::make_pair(uid1,_next_rid));
      _users.insert(std::make_pair(uid2,_next_rid));
      _rooms.insert(std::make_pair(_next_rid,rp));

      //5.更新下一个房间ID
      _next_rid++;

      //6.返回房间对象
      return rp;
   }

   //4.实现：“通过房间ID返回房间对象”
   std::shared_ptr<room> get_room_by_rid(uint64_t rid)
   {
      //1.加上互斥锁
      std::unique_lock<std::mutex> lock(_mutex);

      //2.通过房间ID在房间映射表中寻找对应房间对象
      auto it =_rooms.find(rid);
      
      //3.判断寻找结果
      if(it==_rooms.end())
      {
         DLOG("未找到房间对象");
         return std::shared_ptr<room>();
      }
      return it->second;
   }


   //5.实现：“通过用户ID返回其所属的游戏房间对象”
   std::shared_ptr<room> get_room_by_uid(uint64_t uid)
   {
      //1.加互斥锁
      std::unique_lock<std::mutex> lock(_mutex);

      //2.先通过用户映射表找到房间ID
      auto uit=_users.find(uid);
      if(uit==_users.end())
      {
         DLOG("未找到房间ID");
         return std::shared_ptr<room>();
      }

      //3.再通过房间映射表找到房间对象
      auto rit=_rooms.find(uit->second);
      if(rit==_rooms.end())
      {
         DLOG("未找到房间对象");
         return std::shared_ptr<room>();
      }

      //3.直接返回即可
      return rit->second;
   }

   //6.实现：“通过房间ID销毁房间对象”
   /* 简单的说明一下就是：
        1.首先这是一个删除操作，所以说这个函数是没有返回值的
        2.接着就是它要使用房间ID销毁房间对象，所以房间ID是传入的参数

      接着我们就可以想一想，什么才是销毁房间对象：
        1.在房间映射表中将该rid的条目清理掉
        但是我们还要能想到：
        2.在用户映射表中将和该rid关联的条目也清理掉     
   */
   void remove_room(uint64_t rid)
   {
      //1.首先依据房间ID获取房间对象
      std::shared_ptr<room> rp=get_room_by_rid(rid);
      if(rp==nullptr)
      {
         DLOG("房间对象不存在");
         return;
      }

      //2.找到在用户映射表中将和该rid关联的用户ID
      uint64_t uid1=rp->get_white_id();
      uint64_t uid2=rp->get_black_id();

      //3.加互斥锁
      std::unique_lock<std::mutex> lock(_mutex);

      //3.在用户映射表中将和该rid关联的条目也清理掉    
      _users.erase(uid1);
      _users.erase(uid2);

      //2.在房间映射表中将该rid的条目清理掉
      _rooms.erase(rid);
   }


   //7.实现：“移除房间中的指定的用户,当房间中的用户数量为零时销毁房间”
   void remove_room_user(uint64_t uid)
   //先盘点一下思路：
   /*
       //1.销毁用户映射表中该用户的条目
       //2.检查房间中玩家数量
       //3.销毁房间映射表
   */
   {
      /*
      //1.首先获取房间对象
      std::shared_ptr<room> rp=get_room_by_rid(uid);

      //2.加锁
      std::unique_lock<std::mutex> lock(_mutex);

      //3.销毁用户映射表中该用户的条目
      _users.erase(uid);

      //4.获取房间中玩家数量
      int player_count=rp->get_player_count();
      
      uint64_t rid=rp->get_room_id();
      //5.销毁房间映射表
      if(player_count==0)
      {
         _rooms.erase(rid);
      }

      return ;
      */

      //1.获取要销毁的房间智能指针（验证房间是否存在）
      std::shared_ptr<room> rp = get_room_by_uid(uid);
      if (rp.get() == nullptr)
      {
         return; 
      }

      //2.处理玩家退出游戏房间的业务逻辑
      rp->handle_exit(uid);
      
      //3.如果房间中已经没有玩家了就销毁该房间
      if(rp->get_player_count()<=0)
      {
         remove_room(rp->get_room_id());
      }
      return ;
   }
};



#endif
