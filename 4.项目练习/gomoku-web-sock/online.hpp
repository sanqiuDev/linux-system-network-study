#ifndef ONLINE_HPP
#define ONLINE_HPP

#include "util.hpp"

#include <unordered_map>

//核心任务是实现一个在线管理类：class online_manager
class online_manager
{
    private:
    /*
        online_manager = 在线用户管理器
        锁：保证多人同时操作不乱
        大厅用户表：记住谁在大厅
        房间用户表：记住谁在房间

        为什么大厅表 和 房间表 长得一模一样？结构一模一样！key：用户 ID  value：连接指针
        答案超级简单：因为它们存的数据完全一样，只是【人待的地方不一样】！
        
    */

    //1.互斥锁
    std::mutex _mutex;

    //2.大厅用户表：用户ID ---> websocket连接指针
    std::unordered_map<uint64_t,wsserver_t::connection_ptr> _hall_user;
    //注意：这里的wsserver_t::connection_ptr是websocket的连接指针
    //和会话管理那里的wsserver_t::timer_ptr这个websocket的定时器指针是类似的

    //3.房间用户表：房间ID ---> websocket连接指针
    std::unordered_map<uint64_t,wsserver_t::connection_ptr> _room_user;

    public:
    //1.实现构造函数 ---> 确实也没啥构造的，不写也没事会有默认的生成
    online_manager()
    {
        DLOG("在线管理对象创建成功");
    }

    //2.实现：“析构函数”
    ~online_manager()
    {
        DLOG("在线管理对象析构成功");
    }

    //3.实现：“进入游戏大厅”
    void enter_game_hall(uint64_t uid,wsserver_t::connection_ptr &conn)
    /*
        进入游戏大厅的本质就是：在大厅用户表当中建立“用户ID -> websocket连接指针”这样映射
        可以想到我们需要两个参数：1）用户ID 2）websocket连接指针
    */
   {
      //1.加互斥锁
      std::unique_lock<std::mutex> lock(_mutex);
       
      //1.将映射关系添加大厅用户表当中
      _hall_user.insert(std::make_pair(uid,conn));
   }


   //4.实现：“进入游戏房间”
   void enter_game_room(uint64_t uid,wsserver_t::connection_ptr &conn)
   {
     //1.加互斥锁
     std::unique_lock<std::mutex> lock(_mutex);

     //2.将映射关系添加到房间用户表当中
     _room_user.insert(std::make_pair(uid,conn));
   }


   //5.实现：“退出游戏大厅”
   void exit_game_hall(uint64_t uid)
   /*
      反向类比，既然进入是添加到大厅用户表，那么退出就是删除这个条目
      删除这个条目只需要用户ID既可以
   */
  {
    //1.加锁
    std::unique_lock<std::mutex> lock(_mutex);

    //2.根据键直接删除掉该条目
    _hall_user.erase(uid);
  }

  //6.实现：“退出游戏房间”
  void exit_game_room(uint64_t uid)
  {
    //1.加锁
    std::unique_lock<std::mutex> lock(_mutex);

    //2.根据键直接删除掉该条目
    _room_user.erase(uid);
  }

  //7.实现：“判断用户是否在游戏大厅中”
  bool is_in_game_hall(uint64_t uid)
  /*
     这个也好理解：本质就是拿着用户ID在大厅用户表中查找在映射条目是否存在
  */
  {
    //1.加锁
    std::unique_lock<std::mutex> lock(_mutex);

    //2.在大厅用户表中进行查找
    auto it = _hall_user.find(uid);

    //3.
    if(it==_hall_user.end())
    {
        return false;
    }
    return true;
  }

  //8.实现：“判断用户是否在游戏房间中”
  bool is_in_game_room(uint64_t uid)
  {
    //1.加锁
    std::unique_lock<std::mutex> lock(_mutex);

    //2.在房间用户表中进行查找
    auto it = _room_user.find(uid);

    //3.
    if(it==_room_user.end())
    {
        return false;
    }
    return true;
  }

  //9.实现：“从游戏大厅中根据用户ID获取对应的websocket连接”
  wsserver_t::connection_ptr get_conn_from_hall(uint64_t uid)
  {
    //1.加锁
    std::unique_lock<std::mutex> lock(_mutex);

    //2.在大厅用户表中进行查找
    auto it = _hall_user.find(uid);
    if(it==_hall_user.end())
    {
        ELOG("用户不在游戏大厅中");
        return wsserver_t::connection_ptr();
    }
    return it->second;
  }

  //10.实现：“从游戏房间中根据用户ID获取对应的websocket连接”
  wsserver_t::connection_ptr get_conn_from_room(uint64_t uid)
  {
    //1.加锁
    std::unique_lock<std::mutex> lock(_mutex);

    //2.在房间用户表中进行查找
    auto it = _room_user.find(uid);
    if(it==_room_user.end())
    {
        ELOG("用户不在游戏房间中");
        return wsserver_t::connection_ptr();
    }
    return it->second;
  }
};

#endif