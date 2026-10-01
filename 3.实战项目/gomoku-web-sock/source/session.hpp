// 头文件保护宏：防止该头文件被重复包含，避免类/宏/容器重定义编译错误
#ifndef __M_SS_H__
#define __M_SS_H__

#include "util.hpp"      // 引入自定义工具类头文件（包含WebSocket服务器类型定义等核心依赖）
#include <unordered_map> // 引入C++标准无序映射容器库，用于高效存储会话ID与会话对象的键值对
#include <websocketpp/server.hpp>             // 引入WebSocket++服务器核心头文件（提供WebSocket服务器、定时器等核心功能）
#include <websocketpp/config/asio_no_tls.hpp> // 引入WebSocket++的ASIO无TLS配置头文件（基于ASIO的非加密通信配置）

/************************ 会话状态枚举类型定义 ************************/
// 定义用户会话的两种核心状态，区分用户是否完成登录
typedef enum 
{
    UNLOGIN,  // 未登录状态（会话已创建，但用户未完成登录认证）
    LOGIN     // 已登录状态（用户完成登录，会话绑定用户ID）
} ss_statu;

/************************ 会话类（单个会话的核心数据封装） ************************/
/**
 * @brief 会话（Session）类
 * 封装单个客户端的会话信息，包含会话标识、用户绑定、登录状态、定时器关联等核心属性
 * 特性：1. 绑定用户ID与会话ID；2. 标记用户登录状态；3. 关联WebSocket++定时器，控制会话生命周期
 */
class session 
{
    private:
        uint64_t _ssid;              // 会话唯一标识ID（全局唯一，由session_manager分配）
        uint64_t _uid;               // 会话绑定的用户ID（未登录时为0，登录后赋值）
        ss_statu _statu;             // 会话对应的用户状态（UNLOGIN/LOGIN）
        wsserver_t::timer_ptr _tp;   // 会话关联的定时器智能指针（用于控制会话自动销毁）

    public:
        /**
         * @brief 构造函数：初始化会话对象，分配会话ID
         * @param ssid 会话唯一标识ID（由session_manager生成）
         */
        session(uint64_t ssid): _ssid(ssid)
        { 
            DLOG("SESSION %p 被创建！！", this);  // 打印调试日志，输出会话对象内存地址
        }

        /**
         * @brief 析构函数：销毁会话对象，打印销毁日志
         */
        ~session() 
        { 
            DLOG("SESSION %p 被释放！！", this);  // 打印调试日志，输出会话对象内存地址
        }

        /************************ 简单get/set方法（会话信息访问/修改） ************************/
        uint64_t ssid() { return _ssid; }                           // 获取会话ID
        void set_statu(ss_statu statu) { _statu = statu; }          // 设置会话状态（登录/未登录）

        uint64_t get_user() { return _uid; }                        // 获取会话绑定的用户ID
        void set_user(uint64_t uid) { _uid = uid; }                 // 绑定会话到指定用户ID

        wsserver_t::timer_ptr& get_timer() { return _tp; }          // 获取会话关联的定时器（引用返回，避免拷贝）
        void set_timer(const wsserver_t::timer_ptr &tp) { _tp = tp;}// 设置会话关联的定时器

        bool is_login() { return (_statu == LOGIN); }               // 判断用户是否已登录（状态为LOGIN则返回true）
};

/************************ 全局常量定义（会话配置） ************************/
#define SESSION_TIMEOUT 30000  // 会话默认超时时间：30秒（30000毫秒），无通信时自动销毁
#define SESSION_FOREVER -1     // 会话永久存在标记：-1，用于 游戏大厅/房间场景，取消自动销毁

// 类型别名：简化session类的共享智能指针声明，方便后续使用（自动管理会话内存，避免内存泄漏）
using session_ptr = std::shared_ptr<session>;

/************************ 会话管理器类（全局会话的统一管理） ************************/
/**
 * @brief 会话管理器类
 * 封装全局所有会话的创建、查询、销毁、定时器控制等核心功能
 * 特性：1. 自动生成唯一会话ID；2. 基于WebSocket++定时器实现会话生命周期管理；3. 互斥锁保证并发线程安全；4. 支持会话临时/永久状态切换
 */
class session_manager 
{
    private:
        uint64_t _next_ssid;                                  // 下一个要分配的会话ID（自增计数器，保证会话ID全局唯一）
        std::mutex _mutex;                                    // 全局互斥锁，保护所有容器的并发操作（防止多线程数据混乱）
        std::unordered_map<uint64_t, session_ptr> _session;   // 会话映射表：会话ID → 会话智能指针（管理所有有效会话）
        wsserver_t *_server;                                  // WebSocket服务器指针（用于创建/取消定时器，控制会话生命周期）

    public:
        /**
         * @brief 构造函数：初始化会话管理器，初始化会话ID计数器
         * @param srv WebSocket服务器指针（用于操作定时器）
         */
        session_manager(wsserver_t *srv): _next_ssid(1), _server(srv)
        {
            DLOG("session管理器初始化完毕！");
        }

        /**
         * @brief 析构函数：打印会话管理器销毁日志
         */
        ~session_manager() 
        { 
            DLOG("session管理器即将销毁！"); 
        }

        /**
         * @brief 业务接口：创建新会话，绑定用户ID和登录状态
         * @param uid 要绑定的用户ID（未登录时可传0）
         * @param statu 会话初始状态（UNLOGIN/LOGIN）
         * @return session_ptr 新建会话的智能指针
         */
        session_ptr create_session(uint64_t uid, ss_statu statu) 
        {
            //1.加互斥锁，保护会话映射表的并发插入操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.创建新会话对象，分配当前自增的会话ID
            session_ptr ssp(new session(_next_ssid));

            //3.设置会话状态和绑定的用户ID
            ssp->set_statu(statu);
            ssp->set_user(uid);

            //4.将新会话加入映射表，维护全局状态
            _session.insert(std::make_pair(_next_ssid, ssp));

            //5.更新下一个会话ID（自增，保证唯一性）
            _next_ssid++;

            //6.返回新建会话的智能指针
            return ssp;
        }

        /**
         * @brief 辅助接口：将已存在的会话重新加入管理器（定时器取消后复用）
         * @param ssp 要添加的会话智能指针
         * @return void 无返回值
         * 说明：定时器取消时会触发会话删除，需重新添加会话以避免误销毁
         */
        void append_session(const session_ptr &ssp) 
        {
            //1.加互斥锁，保护会话映射表的并发插入操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.将会话重新加入映射表（覆盖原有记录，保证会话不被误删）
            _session.insert(std::make_pair(ssp->ssid(), ssp));
        }

        /**
         * @brief 查询接口：通过会话ID获取会话智能指针
         * @param ssid 会话ID
         * @return session_ptr 对应会话的智能指针（未找到返回空智能指针）
         */
        session_ptr get_session_by_ssid(uint64_t ssid) 
        {
            //1.加互斥锁，保护会话映射表的并发查询操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.查找会话ID对应的记录
            auto it = _session.find(ssid);
            if (it == _session.end()) 
            {
                return session_ptr();  
            }

            //3.返回对应会话的智能指针
            return it->second;
        }

        /**
         * @brief 业务接口：通过会话ID销毁会话（从映射表中移除）
         * @param ssid 要销毁的会话ID
         * @return void 无返回值
         * 说明：定时器超时/用户主动退出时调用，移除会话后智能指针计数-1，无引用则自动销毁
         */
        void remove_session(uint64_t ssid) 
        {
            //1.加互斥锁，保护会话映射表的并发删除操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.从映射表中移除会话记录
            _session.erase(ssid);
        }

        /**
         * @brief 核心业务接口：设置会话过期时间，控制会话生命周期
         * @param ssid 要设置的会话ID
         * @param ms 过期时间（毫秒）：SESSION_TIMEOUT(30000)=临时会话，SESSION_FOREVER(-1)=永久会话
         * @return void 无返回值
         * 核心逻辑：
         *  1. 登录后：会话为临时状态，30秒无通信自动销毁
         *  2. 进入大厅/房间：会话设为永久状态，取消自动销毁
         *  3. 退出大厅/房间：会话恢复临时状态，重新设置30秒超时
         */
        void set_session_expire_time(uint64_t ssid, int ms) 
        {
            //1.通过会话ID获取会话智能指针（验证会话是否存在）
            session_ptr ssp = get_session_by_ssid(ssid);
            if (ssp.get() == nullptr) 
            {
                return;  
            }

            //2.获取会话当前关联的定时器
            wsserver_t::timer_ptr tp = ssp->get_timer();

            /************************ 场景1：会话无定时器 + 设置永久存在 ************************/
            // 会话当前无定时任务，且要设置为永久存在 → 无需操作，直接返回
            if (tp.get() == nullptr && ms == SESSION_FOREVER) 
            { 
                return ;
            }
            /************************ 场景2：会话无定时器 + 设置临时超时 ************************/
            // 会话当前无定时任务，要设置为临时状态（指定时间后销毁）
            else if (tp.get() == nullptr && ms != SESSION_FOREVER) 
            {
                //1.创建WebSocket++定时器：ms毫秒后执行remove_session，销毁该会话
                wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms, 
                    std::bind(&session_manager::remove_session, this, ssid));

                //2.将定时器绑定到会话，便于后续取消/重置
                ssp->set_timer(tmp_tp);
            }
            /************************ 场景3：会话有定时器 + 设置永久存在 ************************/
            // 会话当前有定时任务，要设置为永久存在（取消自动销毁）
            else if (tp.get() != nullptr && ms == SESSION_FOREVER) 
            {
                //1.取消当前定时器：WebSocket++的timer::cancel()会立即执行定时任务（导致会话被删除）
                tp->cancel();

                //2.清空会话关联的定时器，标记为无定时任务
                ssp->set_timer(wsserver_t::timer_ptr());

                //3.延迟0毫秒重新添加会话：避免cancel()触发的remove_session删除有效会话
                //    （0毫秒定时器会在当前事件循环结束后立即执行，保证会话重新加入映射表）
                _server->set_timer(0, std::bind(&session_manager::append_session, this, ssp));
            }
            /************************ 场景4：会话有定时器 + 重置临时超时 ************************/
            // 会话当前有定时任务，要重置超时时间（更新销毁时间）
            else if (tp.get() != nullptr && ms != SESSION_FOREVER) 
            {
                //1.取消原有定时器（避免原有定时任务误删会话）
                tp->cancel();

                //2.清空会话关联的定时器
                ssp->set_timer(wsserver_t::timer_ptr());

                //3.延迟0毫秒重新添加会话，恢复会话有效性
                _server->set_timer(0, std::bind(&session_manager::append_session, this, ssp));

                //4.创建新的定时器，设置新的超时时间
                wsserver_t::timer_ptr tmp_tp  = _server->set_timer(ms, 
                    std::bind(&session_manager::remove_session, this, ssp->ssid()));

                //5.将新定时器绑定到会话
                ssp->set_timer(tmp_tp);
            }
        }
        
};

// 结束头文件保护宏
#endif