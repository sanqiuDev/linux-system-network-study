// 头文件保护宏：防止该头文件被重复包含，避免类/容器重定义编译错误
#ifndef __M_ONLINE_H__
#define __M_ONLINE_H__

#include "util.hpp"      // 引入自定义工具类头文件 ---> 包含WebSocket服务器类型定义等核心依赖
#include <mutex>         // 引入C++标准互斥锁库 ------> 用于实现线程安全，保护并发场景下的容器操作
#include <unordered_map> // 引入C++标准无序映射容器库 -> 用于高效存储用户ID与通信连接的键值对（查询/插入/删除效率更高）

/**
 * @brief 在线用户管理器类
 * 封装了五子棋项目中「游戏大厅」和「游戏房间」的在线用户管理，核心功能是维护用户ID与WebSocket连接的映射关系
 * 特性：1. 分两个容器管理大厅/房间用户，隔离不同场景的在线状态；2. 提供完整的增删查接口；3. 互斥锁保证并发线程安全
 */
class online_manager
{
   private:
        //1.全局互斥锁，保护所有容器的并发操作（防止多线程同时读写容器导致数据混乱、迭代器失效）
        std::mutex _mutex;  

        //2.游戏大厅在线用户映射表：建立「用户唯一ID」与「WebSocket连接指针」的一一对应关系
        // 键（uint64_t）：用户唯一标识uid（无符号64位整数，保证不会溢出）
        // 值（wsserver_t::connection_ptr）：WebSocket连接智能指针，自动管理连接资源，避免内存泄漏
        std::unordered_map<uint64_t, wsserver_t::connection_ptr> _hall_user;

        //3.游戏房间在线用户映射表：与大厅映射表结构一致，专门管理已进入游戏房间的用户
        std::unordered_map<uint64_t, wsserver_t::connection_ptr> _room_user; // 设计目的：隔离大厅和房间的用户状态，方便单独维护（比如：用户退出房间但不退出大厅）

   public:
        /**
         * @brief 业务接口：用户进入游戏大厅，添加大厅在线用户记录
         * @param uid 用户唯一标识ID
         * @param conn WebSocket连接智能指针（引用传递，避免智能指针拷贝开销）
         * @return void 无返回值
         * 说明：仅在用户WebSocket连接建立、成功进入大厅时调用
         */
        void enter_game_hall(uint64_t uid, wsserver_t::connection_ptr &conn) 
        {
            //1.加互斥锁：使用std::unique_lock实现自动加锁/解锁，异常安全（避免手动解锁遗漏导致死锁）
            std::unique_lock<std::mutex> lock(_mutex);

            //2.向大厅用户映射表中插入键值对：uid -> conn
            _hall_user.insert(std::make_pair(uid, conn)); // std::make_pair：构造与unordered_map匹配的键值对对象，高效插入
        }

        /**
         * @brief 业务接口：用户进入游戏房间，添加房间在线用户记录
         * @param uid 用户唯一标识ID
         * @param conn WebSocket连接智能指针（引用传递，避免智能指针拷贝开销）
         * @return void 无返回值
         * 说明：仅在用户成功匹配对手、进入游戏房间时调用
         */
        void enter_game_room(uint64_t uid, wsserver_t::connection_ptr &conn) 
        {
            //1.加互斥锁，保护房间用户映射表的并发插入操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.向房间用户映射表中插入键值对：uid -> conn
            _room_user.insert(std::make_pair(uid, conn));
        }

        /**
         * @brief 业务接口：用户退出游戏大厅，删除大厅在线用户记录
         * @param uid 用户唯一标识ID
         * @return void 无返回值
         * 说明：仅在用户WebSocket连接断开、主动退出大厅时调用
         */
        void exit_game_hall(uint64_t uid) 
        {
            //1.加互斥锁，保护大厅用户映射表的并发删除操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.根据uid删除大厅映射表中的对应记录，unordered_map::erase()直接根据键删除，高效且安全
            _hall_user.erase(uid);
        }

        /**
         * @brief 业务接口：用户退出游戏房间，删除房间在线用户记录
         * @param uid 用户唯一标识ID
         * @return void 无返回值
         * 说明：仅在用户对战结束、主动退出房间或连接断开时调用
         */
        void exit_game_room(uint64_t uid) 
        {
            //1.加互斥锁，保护房间用户映射表的并发删除操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.根据uid删除房间映射表中的对应记录
            _room_user.erase(uid);
        }

        /**
         * @brief 状态判断接口：判断指定用户是否在游戏大厅在线
         * @param uid 用户唯一标识ID
         * @return bool 在线返回true，不在线返回false
         */
        bool is_in_game_hall(uint64_t uid) 
        {
            //1.加互斥锁，保护大厅用户映射表的并发查询操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.unordered_map::find()：根据uid查找对应记录，返回迭代器
            auto it = _hall_user.find(uid);
            if (it == _hall_user.end()) 
            {
                return false;
            }
            //3.找到记录，说明用户在大厅在线
            return true;
        }

        /**
         * @brief 状态判断接口：判断指定用户是否在游戏房间在线
         * @param uid 用户唯一标识ID
         * @return bool 在线返回true，不在线返回false
         */
        bool is_in_game_room(uint64_t uid) 
        {
            //1.加互斥锁，保护房间用户映射表的并发查询操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.根据uid查找房间映射表中的对应记录
            auto it = _room_user.find(uid);
            if (it == _room_user.end()) 
            {
                return false;
            }
            //2.2：找到记录，返回true
            return true;
        }

        /**
         * @brief 连接获取接口：从游戏大厅中根据用户ID获取对应的WebSocket连接
         * @param uid 用户唯一标识ID
         * @return wsserver_t::connection_ptr WebSocket连接智能指针（找到返回有效连接，未找到返回空智能指针）
         * 说明：获取连接后可用于向大厅内指定用户推送消息（如匹配成功通知）
         */
        wsserver_t::connection_ptr get_conn_from_hall(uint64_t uid) 
        {
            //1.加互斥锁，保护大厅用户映射表的并发查询操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.根据uid查找大厅映射表中的对应记录
            auto it = _hall_user.find(uid);
            if (it == _hall_user.end()) 
            {
                return wsserver_t::connection_ptr(); //未找到记录，返回空的WebSocket连接智能指针（空指针可安全判断，不会导致程序崩溃）
            }
            //3.找到记录，返回对应的WebSocket连接智能指针（second为键值对的值，即连接指针）
            return it->second;
        }

        /**
         * @brief 连接获取接口：从游戏房间中根据用户ID获取对应的WebSocket连接
         * @param uid 用户唯一标识ID
         * @return wsserver_t::connection_ptr WebSocket连接智能指针（找到返回有效连接，未找到返回空智能指针）
         * 说明：获取连接后可用于向房间内对手推送消息（如落子信息、对战结果）
         */
        wsserver_t::connection_ptr get_conn_from_room(uint64_t uid) 
        {
            //1.加互斥锁，保护房间用户映射表的并发查询操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.根据uid查找房间映射表中的对应记录
            auto it = _room_user.find(uid);
            if (it == _room_user.end()) 
            {
                return wsserver_t::connection_ptr();
            }
            //3.找到记录，返回对应的WebSocket连接智能指针
            return it->second;
        }
};

// 结束头文件保护宏
#endif