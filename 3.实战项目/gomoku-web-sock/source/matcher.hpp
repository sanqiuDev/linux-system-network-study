// 头文件保护宏：防止该头文件被重复包含，避免 类/模板/容器 重定义编译错误
#ifndef __M_MATCHER_H__
#define __M_MATCHER_H__

#include "util.hpp"   // 引入自定义工具类头文件（JSON序列化、WebSocket等工具）
#include "online.hpp" // 引入在线用户管理头文件（获取用户连接、判断用户在线状态）
#include "db.hpp"     // 引入数据库用户表操作头文件（查询用户天梯分数）
#include "room.hpp"   // 引入房间管理头文件（创建游戏房间）

#include <list>               // 引入C++标准链表容器库（用于实现匹配队列，支持中间元素删除）
#include <mutex>              // 引入C++标准互斥锁库（实现线程安全）
#include <condition_variable> // 引入C++标准条件变量库（实现线程阻塞/唤醒）

/************************ 通用匹配队列模板类（线程安全） ************************/
/**
 * @brief 通用匹配队列模板类
 * 封装线程安全的队列操作，支持入队、出队、指定元素删除、阻塞等待等核心功能
 * 模板参数T：队列存储的数据类型（此处用于存储用户ID，uint64_t）
 * 特性：1. 基于std::list实现，支持中间元素删除；2. 互斥锁保证并发安全；3. 条件变量实现阻塞等待
 */
template <class T>
class match_queue 
{
    private:
        std::list<T> _list;             // 底层存储容器：std::list（链表），支持高效的中间元素删除
        std::mutex _mutex;              // 互斥锁：保护_list的所有并发操作，避免数据竞争
        std::condition_variable _cond;  // 条件变量：用于阻塞匹配线程，队列元素不足时等待

    public:
        /**
         * @brief 获取队列中元素个数（线程安全）
         * @return int 队列当前元素数量
         */
        int size() 
        {  
            std::unique_lock<std::mutex> lock(_mutex);  // 加锁，保证size()操作的原子性
            return _list.size(); 
        }

        /**
         * @brief 判断队列是否为空（线程安全）
         * @return bool 空返回true，非空返回false
         */
        bool empty() 
        {
            std::unique_lock<std::mutex> lock(_mutex);  // 加锁，保证empty()操作的原子性
            return _list.empty();
        }

        /**
         * @brief 阻塞当前线程，等待条件变量唤醒
         * 说明：匹配线程调用此方法，当队列元素<2时阻塞，直到有新元素入队被唤醒
         */
        void wait() 
        {
            //1.加锁，条件变量必须配合unique_lock使用
            std::unique_lock<std::mutex> lock(_mutex); 
            
            //2.阻塞线程，释放锁；被唤醒后重新加锁并继续执行
            _cond.wait(lock);  
        }

        /**
         * @brief 入队数据，并唤醒所有阻塞的匹配线程
         * @param data 要入队的数据（用户ID）
         * @return void 无返回值
         */
        void push(const T &data) 
        {
            //1.加锁，保护_list的push_back操作
            std::unique_lock<std::mutex> lock(_mutex);  

            //2.将数据添加到链表尾部（入队）
            _list.push_back(data);          
            
            //3.唤醒所有阻塞在wait()的匹配线程
            _cond.notify_all();                         
        }

        /**
         * @brief 出队数据（从队首取数据）
         * @param data 输出参数，存储出队的数据
         * @return bool 出队成功返回true，队列为空返回false
         */
        bool pop(T &data) 
        {
            //1.加锁，保护_list的pop_front操作
            std::unique_lock<std::mutex> lock(_mutex);  

            //2.队列空，出队失败
            if (_list.empty() == true)                 
            { 
                return false;
            }

            //3.获取队首元素
            data = _list.front();                       

            //4.删除队首元素（出队）
            _list.pop_front();                         
            return true;
        }

        /**
         * @brief 移除队列中指定的数据（支持中间元素删除）
         * @param data 要移除的数据（用户ID）
         * @return void 无返回值
         * 说明：用户取消匹配/掉线时调用，从队列中删除该用户ID
         */
        void remove(T &data) 
        {
            //1.加锁，保护_list的remove操作
            std::unique_lock<std::mutex> lock(_mutex);  

            //2.遍历链表，删除所有等于data的元素
            _list.remove(data);                         
        }
};

/************************ 匹配器类（核心匹配逻辑封装） ************************/
/**
 * @brief 五子棋玩家匹配器类
 * 实现基于天梯分数的分层匹配逻辑，支持三个档次的匹配队列（普通/高手/大神）
 * 特性：1. 多线程处理不同队列的匹配；2. 校验玩家在线状态；3. 匹配成功后创建游戏房间；4. 线程安全的队列操作
 */
class matcher 
{
    private:
        //1.分层匹配队列：根据天梯分数将玩家分配到不同队列，实现同水平玩家匹配
        match_queue<uint64_t> _q_normal;       // 普通玩家队列：天梯分数 < 2000
        match_queue<uint64_t> _q_high;         // 高手玩家队列：2000 ≤ 天梯分数 < 3000
        match_queue<uint64_t> _q_super;        // 大神玩家队列：天梯分数 ≥ 3000

        //2.匹配处理线程：每个队列对应一个独立线程，避免不同档次匹配相互阻塞
        std::thread _th_normal;                // 普通队列处理线程
        std::thread _th_high;                  // 高手队列处理线程
        std::thread _th_super;                 // 大神队列处理线程

        //3.外部依赖指针：通过构造函数注入，降低耦合度
        room_manager *_rm;                     // 房间管理器指针（用于创建游戏房间）
        user_table *_ut;                       // 用户表操作类指针（用于查询玩家天梯分数）
        online_manager *_om;                   // 在线用户管理器指针（用于校验玩家在线状态、获取连接）

    private:
        /**
         * @brief 通用匹配处理函数（核心匹配逻辑）
         * @param mq 要处理的匹配队列（普通/高手/大神队列）
         * @return void 无返回值（死循环，持续处理匹配）
         * 核心逻辑：队列元素≥2时，取出两个玩家，校验状态，创建房间，返回匹配成功响应
         */
        void handle_match(match_queue<uint64_t> &mq) 
        {
            while(1)   // 死循环：持续处理该队列的匹配请求（线程常驻）
            {
                //1.判断队列人数是否≥2，不足则阻塞等待（条件变量唤醒）
                while (mq.size() < 2) 
                {
                    mq.wait();  
                }    

                //2.队列人数足够，出队两个玩家ID
                uint64_t uid1, uid2;
                bool ret = mq.pop(uid1);  
                if (ret == false) 
                {       
                    // 出队失败（队列空），跳过本次循环
                    continue;  
                }
                ret = mq.pop(uid2);     
                if (ret == false) 
                {    
                    // 出队失败，将第一个玩家重新入队，跳过本次循环   
                    this->add(uid1);  
                    continue; 
                }

                //3.校验两个玩家是否在线（必须在游戏大厅在线）
                //3.1：获取第一个玩家的大厅连接，校验是否有效
                wsserver_t::connection_ptr conn1 = _om->get_conn_from_hall(uid1);
                if (conn1.get() == nullptr) 
                {
                    this->add(uid2);  // 玩家1掉线，将玩家2重新入队
                    continue;
                }
                //3.2：获取第二个玩家的大厅连接，校验是否有效
                wsserver_t::connection_ptr conn2 = _om->get_conn_from_hall(uid2);
                if (conn2.get() == nullptr) 
                {
                    this->add(uid1);  // 玩家2掉线，将玩家1重新入队
                    continue;
                }

                //4.两个玩家均在线，为其创建游戏房间
                room_ptr rp = _rm->create_room(uid1, uid2);
                if (rp.get() == nullptr)
                {  
                    this->add(uid1);   // 两个玩家重新入队
                    this->add(uid2);
                    continue;
                }

                //5.向两个玩家发送匹配成功响应
                //5.1：
                Json::Value resp;
                resp["optype"] = "match_success";  
                resp["result"] = true;             
                //5.2：将JSON对象序列化为字符串
                std::string body;
                json_util::serialize(resp, body);  
                //5.3：向两个玩家发送响应消息
                conn1->send(body);
                conn2->send(body);
            }
        }

        /**
         * @brief 普通队列处理线程入口函数
         * 说明：封装handle_match调用，适配std::thread的构造要求
         */
        void th_normal_entry() { return handle_match(_q_normal); }

        /**
         * @brief 高手队列处理线程入口函数
         */
        void th_high_entry() { return handle_match(_q_high); }

        /**
         * @brief 大神队列处理线程入口函数
         */
        void th_super_entry() { return handle_match(_q_super); }

    public:
        /**
         * @brief 构造函数：初始化匹配器，启动三个匹配处理线程
         * @param rm 房间管理器指针
         * @param ut 用户表操作类指针
         * @param om 在线用户管理器指针
         */
        matcher(room_manager *rm, user_table *ut, online_manager *om): 
            _rm(rm), _ut(ut), _om(om),
            // 初始化并启动三个匹配处理线程
            _th_normal(std::thread(&matcher::th_normal_entry, this)),
            _th_high(std::thread(&matcher::th_high_entry, this)),
            _th_super(std::thread(&matcher::th_super_entry, this))
        {
            DLOG("游戏匹配模块初始化完毕....");
        }

        /**
         * @brief 业务接口：添加玩家到对应档次的匹配队列
         * @param uid 要匹配的玩家ID
         * @return bool 添加成功返回true，失败返回false
         */
        bool add(uint64_t uid)
        {
            //1.根据用户ID查询玩家信息（获取天梯分数）
            Json::Value user;
            bool ret = _ut->select_by_id(uid, user);
            if (ret == false) 
            {
                DLOG("获取玩家:%lu 信息失败！！", uid);  // 注意：uid是uint64_t，用%lu格式化
                return false;
            }
            int score = user["score"].asInt();  // 提取天梯分数

            //2.根据天梯分数将玩家添加到对应队列 
            if (score < 2000)  
            {
                _q_normal.push(uid);           
            }
            else if (score >= 2000 && score < 3000) 
            {
                _q_high.push(uid);             
            }
            else              
            {
                _q_super.push(uid);            
            }
            return true;
        }

        /**
         * @brief 业务接口：从对应档次的匹配队列中移除玩家
         * @param uid 要移除的玩家ID
         * @return bool 移除成功返回true，失败返回false
         * 说明：玩家取消匹配/掉线/进入房间时调用，避免无效匹配
         */
        bool del(uint64_t uid) 
        {
            //1.根据用户ID查询玩家信息（获取天梯分数）
            Json::Value user;
            bool ret = _ut->select_by_id(uid, user);
            if (ret == false) 
            {
                DLOG("获取玩家:%lu 信息失败！！", uid);
                return false;
            }
            int score = user["score"].asInt();  // 提取天梯分数

            //2.根据天梯分数从对应队列移除玩家
            if (score < 2000) 
            {
                _q_normal.remove(uid);         
            }
            else if (score >= 2000 && score < 3000) 
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

// 结束头文件保护宏
#endif