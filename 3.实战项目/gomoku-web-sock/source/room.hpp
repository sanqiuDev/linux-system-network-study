// 头文件保护宏：防止该头文件被重复包含，避免类/宏/容器重定义编译错误
#ifndef __M_ROOM_H__
#define __M_ROOM_H__

#include "util.hpp"   // 引入自定义工具类头文件（WebSocket、JSON等工具）
#include "logger.hpp" // 引入自定义日志头文件（打印调试/错误日志）
#include "online.hpp" // 引入在线用户管理头文件（获取用户连接、判断用户在线状态）
#include "db.hpp"     // 引入数据库用户表操作头文件（更新用户对战结果、天梯分数）

/************************ 全局常量定义（五子棋业务配置） ************************/
#define BOARD_ROW 15          // 五子棋棋盘行数（15行）
#define BOARD_COL 15          // 五子棋棋盘列数（15列）
#define CHESS_WHITE 1         // 白棋标识（数值1）
#define CHESS_BLACK 2         // 黑棋标识（数值2）

/************************ 房间状态枚举类型定义 ************************/
// 定义房间的两种核心状态，清晰区分房间是否在进行对战
typedef enum 
{
    GAME_START,  // 游戏进行中（房间有效，玩家可下棋/聊天）
    GAME_OVER    // 游戏结束（房间无效，可销毁）
} room_statu;

/************************ 五子棋房间类（单个房间的业务逻辑封装） ************************/
/**
 * @brief 五子棋房间类
 * 封装了单个房间的核心业务逻辑：棋盘管理、下棋判断、聊天处理、玩家退出、消息广播等
 * 特性：1. 维护棋盘数据和房间状态；2. 实现五子棋胜利判断（五星连珠）；3. 依赖外部用户表和在线管理器；4. 自动管理房间生命周期
 */
class room 
{
    private:
        uint64_t _room_id;                    // 房间唯一标识ID
        room_statu _statu;                    // 房间当前状态（GAME_START/GAME_OVER）
        int _player_count;                    // 房间内当前玩家数量（最大2人）
        uint64_t _white_id;                   // 白棋玩家的用户ID
        uint64_t _black_id;                   // 黑棋玩家的用户ID

        user_table *_tb_user;                 // 用户表操作类指针（用于更新玩家对战结果、天梯分数）
        online_manager *_online_user;         // 在线用户管理器指针（用于获取玩家连接、判断玩家在线状态）
        std::vector<std::vector<int>> _board; // 五子棋棋盘数据（二维向量）：0=空位置，1=白棋，2=黑棋

    private:
        /**
         * @brief 私有辅助方法：沿指定方向检测同色棋子是否达到五子连珠
         * @param row 落子位置的行号
         * @param col 落子位置的列号
         * @param row_off 行方向偏移量（0=水平，1=垂直，-1=斜向）
         * @param col_off 列方向偏移量（0=垂直，1=水平，1/-1=斜向）
         * @param color 棋子颜色（CHESS_WHITE/CHESS_BLACK）
         * @return bool 达到五子及以上返回true，否则返回false
         * 说明：沿指定方向的「正反两个方向」进行检索（如水平方向：左→右 + 右→左）
         */
        bool five(int row, int col, int row_off, int col_off, int color)  //row和col是当前落子位置，row_off和col_off是检索方向（偏移量）
        {
            //1.计数器，初始值为1（当前落子本身算1个）
            int count = 1;  

            //2.沿偏移量正方向检索同色棋子
            int search_row = row + row_off;
            int search_col = col + col_off;
            while(search_row >= 0 && search_row < BOARD_ROW &&
                  search_col >= 0 && search_col < BOARD_COL &&
                  _board[search_row][search_col] == color) 
            // 循环条件：1. 检索位置在棋盘范围内；2. 该位置棋子颜色与当前落子颜色一致
            {
                //2.1：同色棋子数量+1
                count++;  

                //2.2：检索位置继续沿正方向偏移
                search_row += row_off;
                search_col += col_off;
            }

            //3.沿偏移量反方向检索同色棋子（抵消正方向偏移，往回检索）
            search_row = row - row_off;
            search_col = col - col_off;
            while(search_row >= 0 && search_row < BOARD_ROW &&
                  search_col >= 0 && search_col < BOARD_COL &&
                  _board[search_row][search_col] == color) 
            {
                //3.1：同色棋子数量+1
                count++;  

                //3.2：检索位置继续沿反方向偏移
                search_row -= row_off;
                search_col -= col_off;
            }

            //4.判断是否达到五子连珠（count≥5则返回true）
            return (count >= 5);
        }

        /**
         * @brief 私有辅助方法：检查当前落子是否导致玩家胜利（核心胜利判断逻辑）
         * @param row 落子位置的行号
         * @param col 落子位置的列号
         * @param color 棋子颜色（CHESS_WHITE/CHESS_BLACK）
         * @return uint64_t 胜利玩家ID（无胜利者返回0）
         */
        uint64_t check_win(int row, int col, int color) 
        {
            //1.检测四个核心方向是否存在五子连珠，满足任意一个方向即判定胜利
            // 方向1：水平方向（行偏移0，列偏移1）
            // 方向2：垂直方向（行偏移1，列偏移0）
            // 方向3：正斜向（左上→右下，行偏移-1，列偏移1）
            // 方向4：反斜向（右上→左下，行偏移-1，列偏移-1）
            if (five(row, col, 0, 1, color) || 
                five(row, col, 1, 0, color) ||
                five(row, col, -1, 1, color)||
                five(row, col, -1, -1, color)) 
            {
                //1.1：存在五子连珠，返回对应胜利玩家的ID（白棋→白棋玩家ID，黑棋→黑棋玩家ID）
                return color == CHESS_WHITE ? _white_id : _black_id;
            }

            //2.无五子连珠，返回0表示暂无胜利者
            return 0;
        }

    public:
        /**
         * @brief 构造函数：初始化房间对象，创建空棋盘
         * @param room_id 房间唯一标识ID
         * @param tb_user 用户表操作类指针
         * @param online_user 在线用户管理器指针
         */
        room(uint64_t room_id, user_table *tb_user, online_manager *online_user):
            _room_id(room_id), _statu(GAME_START), _player_count(0),
            _tb_user(tb_user), _online_user(online_user),
            _board(BOARD_ROW, std::vector<int>(BOARD_COL, 0))
        {  
            DLOG("%lu 房间创建成功!!", _room_id); // 初始化15x15棋盘，所有位置置0（空）
        }

        /**
         * @brief 析构函数：销毁房间对象，打印销毁日志
         */
        ~room() 
        {
            DLOG("%lu 房间销毁成功!!", _room_id);
        }

        /************************ 简单get/set方法（房间信息访问/修改） ************************/
        uint64_t id() { return _room_id; }                 // 获取房间ID
        room_statu statu() { return _statu; }              // 获取房间当前状态
        int player_count() { return _player_count; }       // 获取房间内玩家数量
        void add_white_user(uint64_t uid) { _white_id = uid; _player_count++; }  // 添加白棋玩家
        void add_black_user(uint64_t uid) { _black_id = uid; _player_count++; }  // 添加黑棋玩家
        uint64_t get_white_user() { return _white_id; }    // 获取白棋玩家ID
        uint64_t get_black_user() { return _black_id; }    // 获取黑棋玩家ID

        /**
         * @brief 业务接口：处理玩家下棋请求
         * @param req 输入参数，Json::Value对象（包含房间ID、玩家ID、落子位置row/col）
         * @return Json::Value 响应结果（包含操作结果、胜利者ID、失败原因等）
         */
        Json::Value handle_chess(Json::Value &req) 
        {
            //1.初始化响应对象，继承请求中的基础信息（保持请求/响应格式一致）
            Json::Value json_resp = req;

            //2.提取请求中的核心数据（落子位置、当前玩家ID）
            int chess_row = req["row"].asInt();
            int chess_col = req["col"].asInt();
            uint64_t cur_uid = req["uid"].asUInt64();

            //3.判断两位玩家是否都在线，任意一方掉线则另一方直接胜利
            if (_online_user->is_in_game_room(_white_id) == false) 
            {
                json_resp["result"] = true;
                json_resp["reason"] = "对方掉线，不战而胜！";
                json_resp["winner"] = (Json::UInt64)_black_id;  // 黑棋玩家胜利
                return json_resp;
            }
            if (_online_user->is_in_game_room(_black_id) == false) 
            {
                json_resp["result"] = true;
                json_resp["reason"] = "对方掉线，不战而胜！";
                json_resp["winner"] = (Json::UInt64)_white_id;  // 白棋玩家胜利
                return json_resp;
            }

            //4.判断落子位置是否合法（是否已被占用）
            if (_board[chess_row][chess_col] != 0) 
            {
                json_resp["result"] = false;
                json_resp["reason"] = "当前位置已经有了其他棋子！";
                return json_resp;
            }



            //5.记录落子信息，更新棋盘数据
            //5.1：判断当前玩家的棋子颜色
            int cur_color = cur_uid == _white_id ? CHESS_WHITE : CHESS_BLACK;  
            //5.2：将棋盘对应位置置为当前棋子颜色
            _board[chess_row][chess_col] = cur_color;                   

            //6.判断当前落子是否导致胜利（检测五子连珠）
            uint64_t winner_id = check_win(chess_row, chess_col, cur_color);
            if (winner_id != 0) 
            {
                json_resp["reason"] = "五星连珠，战无敌！";  // 胜利提示信息
            }

            //7.填充响应结果，返回给调用者
            json_resp["result"] = true;
            json_resp["winner"] = (Json::UInt64)winner_id;
            return json_resp;
        }

        /**
         * @brief 业务接口：处理玩家聊天请求
         * @param req 输入参数，Json::Value对象（包含房间ID、玩家ID、聊天消息message）
         * @return Json::Value 响应结果（包含操作结果、失败原因等）
         */
        Json::Value handle_chat(Json::Value &req) 
        {
            //1.初始化响应对象，继承请求中的基础信息
            Json::Value json_resp = req;

            //2.提取聊天消息，进行敏感词检测（简单示例：检测"垃圾"）
            std::string msg = req["message"].asString();
            size_t pos = msg.find("垃圾");
            if (pos != std::string::npos) 
            {
                json_resp["result"] = false;
                json_resp["reason"] = "消息中包含敏感词，不能发送！";
                return json_resp;
            }

            //3.聊天消息合法，返回成功响应（后续由broadcast广播给对方玩家）
            json_resp["result"] = true;
            return json_resp;
        }

        /**
         * @brief 业务接口：处理玩家退出房间请求
         * @param uid 退出房间的玩家ID
         * @return void 无返回值
         * 说明：玩家掉线/主动退出时调用，处理对战结果并广播给另一方玩家
         */
        void handle_exit(uint64_t uid) 
        {
            //1.初始化响应对象，继承请求中的基础信息
            Json::Value json_resp;

            //2.判断房间是否在游戏进行中，若是则判定对方胜利
            if (_statu == GAME_START) 
            {
                //2.1：确定胜利者ID（退出玩家的对手即为胜利者）
                uint64_t winner_id = (uid == _white_id ? _black_id : _white_id);

                //2.2：构造退出响应消息（模拟下棋结果响应格式）
                json_resp["optype"] = "put_chess";
                json_resp["result"] = true;
                json_resp["reason"] = "对方掉线，不战而胜！";
                json_resp["room_id"] = (Json::UInt64)_room_id;
                json_resp["uid"] = (Json::UInt64)uid;
                json_resp["row"] = -1;  // 无效落子位置（标识为掉线胜利）
                json_resp["col"] = -1;
                json_resp["winner"] = (Json::UInt64)winner_id;

                //2.3：更新数据库中玩家的对战结果（胜利者加分，失败者扣分）
                uint64_t loser_id = winner_id == _white_id ? _black_id : _white_id;
                _tb_user->win(winner_id);
                _tb_user->lose(loser_id);

                //2.4：更新房间状态为游戏结束
                _statu = GAME_OVER;

                //2.5：广播退出结果给另一方玩家
                broadcast(json_resp);
            }

            //3.减少房间内玩家数量（无论是否游戏中，都需更新玩家计数）
            _player_count--;
            return;
        }

        /**
         * @brief 核心业务接口：统一处理房间内所有请求（分发到具体处理方法）
         * @param req 输入参数，Json::Value对象（包含optype字段标识请求类型）
         * @return void 无返回值
         * 说明：请求类型支持"put_chess"（下棋）、"chat"（聊天），其他为未知类型
         */
        void handle_request(Json::Value &req) 
        {
            //1.初始化响应对象
            Json::Value json_resp;

            //2.校验房间号是否匹配（防止请求发送到错误房间）
            uint64_t room_id = req["room_id"].asUInt64();
            if (room_id != _room_id) 
            {
                json_resp["optype"] = req["optype"].asString();
                json_resp["result"] = false;
                json_resp["reason"] = "房间号不匹配！";
                return broadcast(json_resp);
            }

            //3.根据请求类型（optype）分发到具体处理方法
            std::string optype = req["optype"].asString();
            //情况一：处理下棋请求
            if (optype == "put_chess") 
            {
                json_resp = handle_chess(req);
                if (json_resp["winner"].asUInt64() != 0)  //若下棋请求产生胜利者，更新数据库并修改房间状态
                {
                    uint64_t winner_id = json_resp["winner"].asUInt64();
                    uint64_t loser_id = winner_id == _white_id ? _black_id : _white_id;
                    _tb_user->win(winner_id);
                    _tb_user->lose(loser_id);
                    _statu = GAME_OVER;
                }
            }
            //情况二：处理聊天请求
            else if (optype == "chat") 
            {
                
                json_resp = handle_chat(req);
            }
            //情况三：处理未知请求类型
            else 
            {
                json_resp["optype"] = optype;
                json_resp["result"] = false;
                json_resp["reason"] = "未知请求类型";
            }

            //4.序列化响应结果，打印调试日志
            std::string body;
            json_util::serialize(json_resp, body);
            DLOG("房间-广播动作: %s", body.c_str());

            //5.广播响应结果给房间内所有玩家
            return broadcast(json_resp);
        }

        /**
         * @brief 辅助接口：将响应消息广播给房间内的两位玩家
         * @param rsp 要广播的响应消息（Json::Value对象）
         * @return void 无返回值
         * 说明：通过在线管理器获取玩家WebSocket连接，发送序列化后的JSON字符串
         */
        void broadcast(Json::Value &rsp) 
        {
            //1.将Json::Value响应消息序列化为JSON格式字符串
            std::string body;
            json_util::serialize(rsp, body);

            //2.获取白棋玩家的WebSocket连接，发送消息
            wsserver_t::connection_ptr wconn = _online_user->get_conn_from_room(_white_id);
            if (wconn.get() != nullptr) 
                wconn->send(body);
            else 
                DLOG("房间-白棋玩家连接获取失败");

            //3.获取黑棋玩家的WebSocket连接，发送消息
            wsserver_t::connection_ptr bconn = _online_user->get_conn_from_room(_black_id);
            if (bconn.get() != nullptr)
                bconn->send(body);
            else 
                DLOG("房间-黑棋玩家连接获取失败");

            return;
        }
};

// 类型别名：简化room类的共享智能指针声明，方便后续使用（自动管理房间内存，避免内存泄漏）
using room_ptr = std::shared_ptr<room>;

/************************ 房间管理器类（全局房间的统一管理） ************************/
/**
 * @brief 房间管理器类
 * 封装了全局所有房间的创建、查询、销毁、用户与房间的绑定等功能
 * 特性：1. 维护房间与用户的映射关系；2. 自动生成唯一房间ID；3. 互斥锁保证并发线程安全；4. 智能指针管理房间生命周期
 */
class room_manager
{
    private:
        uint64_t _next_rid;                            // 下一个要分配的房间ID（自增计数器，保证房间ID唯一）
        std::mutex _mutex;                             // 全局互斥锁，保护所有容器的并发操作（防止多线程数据混乱）
        user_table *_tb_user;                          // 用户表操作类指针（传递给新建房间，用于更新对战结果）
        online_manager *_online_user;                  // 在线用户管理器指针（传递给新建房间，用于获取用户连接）
        std::unordered_map<uint64_t, room_ptr> _rooms; // 房间映射表：房间ID → 房间智能指针（管理所有有效房间）
        std::unordered_map<uint64_t, uint64_t> _users; // 用户-房间映射表：用户ID → 房间ID（快速通过用户查找所属房间）

    public:
        /**
         * @brief 构造函数：初始化房间管理器，初始化房间ID计数器
         * @param ut 用户表操作类指针
         * @param om 在线用户管理器指针
         */
        room_manager(user_table *ut, online_manager *om):
            _next_rid(1), _tb_user(ut), _online_user(om) 
        {
            DLOG("房间管理模块初始化完毕！");
        }

        /**
         * @brief 析构函数：打印房间管理器销毁日志
         */
        ~room_manager() { DLOG("房间管理模块即将销毁！"); }

        /**
         * @brief 业务接口：为两位匹配成功的玩家创建新房间
         * @param uid1 玩家1 ID（白棋玩家）
         * @param uid2 玩家2 ID（黑棋玩家）
         * @return room_ptr 新建房间的智能指针（创建失败返回空智能指针）
         */
        room_ptr create_room(uint64_t uid1, uint64_t uid2) 
        {
            //1.校验两位玩家是否都在游戏大厅中（仅大厅内玩家可创建房间）
            if (_online_user->is_in_game_hall(uid1) == false) 
            {
                DLOG("用户：%lu 不在大厅中，创建房间失败!", uid1);
                return room_ptr();  // 返回空智能指针表示创建失败
            }
            if (_online_user->is_in_game_hall(uid2) == false) 
            {
                DLOG("用户：%lu 不在大厅中，创建房间失败!", uid2);
                return room_ptr();  
            }

            //2.加互斥锁，保护 房间/用户映射表 的并发插入操作
            std::unique_lock<std::mutex> lock(_mutex);

            //3.创建新房间，添加两位玩家（uid1=白棋，uid2=黑棋）
            room_ptr rp(new room(_next_rid, _tb_user, _online_user));
            rp->add_white_user(uid1);
            rp->add_black_user(uid2);

            //4.将新房间和用户-房间关系加入映射表，维护全局状态
            _rooms.insert(std::make_pair(_next_rid, rp));    // 房间ID → 房间智能指针
            _users.insert(std::make_pair(uid1, _next_rid));  // 玩家1 → 房间ID
            _users.insert(std::make_pair(uid2, _next_rid));  // 玩家2 → 房间ID

            //5.更新下一个房间ID（自增，保证唯一性）
            _next_rid++;

            //6.返回新建房间的智能指针
            return rp;
        }

        /**
         * @brief 查询接口：通过房间ID获取房间智能指针
         * @param rid 房间ID
         * @return room_ptr 对应房间的智能指针（未找到返回空智能指针）
         */
        room_ptr get_room_by_rid(uint64_t rid) 
        {
            //1.加互斥锁，保护房间映射表的并发查询操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.通过房间ID查找对应房间的智能的指针
            auto it = _rooms.find(rid);
            if (it == _rooms.end()) 
            {
                return room_ptr();  
            }

            //3.返回对应房间的智能指针
            return it->second;  
        }

        /**
         * @brief 查询接口：通过用户ID获取所属房间的智能指针
         * @param uid 用户ID
         * @return room_ptr 所属房间的智能指针（未找到返回空智能指针）
         */
        room_ptr get_room_by_uid(uint64_t uid) 
        {
            //1.加互斥锁，保护 用户-房间映射表/房间映射表 的并发查询操作
            std::unique_lock<std::mutex> lock(_mutex);

            //2.通过用户ID查找对应的房间ID
            auto uit = _users.find(uid);
            if (uit == _users.end()) 
            {
                return room_ptr();  // 该用户未在任何房间中，返回空智能指针
            }
            uint64_t rid = uit->second;

            //3.通过房间ID查找对应的房间智能指针
            auto rit = _rooms.find(rid);
            if (rit == _rooms.end()) 
            {
                return room_ptr();  // 房间不存在，返回空智能指针
            }

            //4.返回房间智能指针
            return rit->second;
        }

        /**
         * @brief 业务接口：通过房间ID销毁房间（清理映射表，释放房间内存）
         * @param rid 房间ID
         * @return void 无返回值
         */
        void remove_room(uint64_t rid) 
        {
            //1.获取要销毁的房间智能指针（验证房间是否存在）
            room_ptr rp = get_room_by_rid(rid);
            if (rp.get() == nullptr) 
            {
                return;  // 房间不存在，直接返回
            }

            //2.提取房间内的两位玩家ID
            uint64_t uid1 = rp->get_white_user();
            uint64_t uid2 = rp->get_black_user();

            //3.加互斥锁，保护映射表的并发删除操作
            std::unique_lock<std::mutex> lock(_mutex);

            //4.清理用户-房间映射表（移除两位玩家的记录）
            _users.erase(uid1);
            _users.erase(uid2);

            //5.清理房间映射表（移除房间记录，shared_ptr计数器-1，无其他引用则自动释放房间内存）
            _rooms.erase(rid);
        }

        /**
         * @brief 业务接口：移除房间内指定用户，若房间无玩家则销毁房间
         * @param uid 要移除的用户ID
         * @return void 无返回值
         * 说明：用户WebSocket连接断开时调用，自动维护房间生命周期
         */
        void remove_room_user(uint64_t uid) 
        {
            //1.获取要销毁的房间智能指针（验证房间是否存在）
            room_ptr rp = get_room_by_uid(uid);
            if (rp.get() == nullptr) 
            {
                return;  // 用户未在任何房间中，直接返回
            }

            //2.处理用户退出房间的业务逻辑（判定对战结果、广播消息等）
            rp->handle_exit(uid);

            //3.若房间内已无玩家，销毁该房间
            if (rp->player_count() == 0) 
            {
                remove_room(rp->id());
            }
            return ;
        }
};

// 结束头文件保护宏
#endif