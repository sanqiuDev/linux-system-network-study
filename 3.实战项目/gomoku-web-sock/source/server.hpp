#ifndef __M_SRV_H__
#define __M_SRV_H__


#include "db.hpp"       // 引入数据库操作头文件（用户表CRUD）
#include "matcher.hpp"  // 引入匹配模块头文件（玩家匹配队列、多线程匹配）
#include "online.hpp"   // 引入在线用户管理头文件（大厅/房间连接管理）
#include "room.hpp"     // 引入房间管理头文件（房间创建、下棋/聊天处理）
#include "session.hpp"  // 引入会话管理头文件（Session创建、生命周期控制）
#include "util.hpp"     // 引入工具类头文件（JSON序列化、文件操作、字符串处理）

/************************ 全局常量定义（服务器配置） ************************/
#define WWWROOT "./wwwroot/"  // 静态资源根目录：存放HTML/CSS/JS等前端文件

/************************ 五子棋服务器核心类 ************************/
/**
 * @brief 五子棋服务器核心类（gobang_server）
 * 整合所有业务模块，实现HTTP/WebSocket双协议处理：
 *  1. HTTP协议：处理静态资源请求、用户注册/登录/信息查询；
 *  2. WebSocket协议：处理游戏大厅/房间长连接、匹配/下棋/聊天请求；
 * 核心依赖：数据库用户表、在线管理器、房间管理器、匹配器、会话管理器
 */
class gobang_server
{
    private:
        std::string _web_root;     // 静态资源根目录（默认./wwwroot/）
        wsserver_t _wssrv;         // WebSocket++服务器对象（同时支持HTTP/WebSocket）

        user_table _ut;            // 用户表操作对象（数据库CRUD）
        online_manager _om;        // 在线用户管理器（大厅/房间连接管理）
        room_manager _rm;          // 房间管理器（创建/销毁房间、处理房间请求）

        matcher _mm;               // 匹配器（分层匹配队列、多线程匹配）
        session_manager _sm;       // 会话管理器（Session创建、生命周期控制）

    private:
        /**
         * @brief HTTP响应封装工具函数（JSON格式响应）
         * @param conn 连接指针
         * @param result 操作结果（true/false）
         * @param code HTTP状态码（200/400/500等）
         * @param reason 响应描述（成功/失败原因）
         * @return void 无返回值
         */
        void http_resp(wsserver_t::connection_ptr &conn, bool result, 
            websocketpp::http::status_code::value code, const std::string &reason) 
        {
            //1.构造JSON响应对象
            Json::Value resp_json;
            resp_json["result"] = result;
            resp_json["reason"] = reason;

            //2.序列化JSON为字符串
            std::string resp_body;
            json_util::serialize(resp_json, resp_body);

            //3.设置HTTP响应（状态码、正文、Content-Type）
            conn->set_status(code);
            conn->set_body(resp_body);
            conn->append_header("Content-Type", "application/json");
            return;
        }

        /**
         * @brief HTTP回调-静态资源请求处理（HTML/CSS/JS等）
         * @param conn WebSocket++连接指针（封装HTTP连接）
         * @return void 无返回值
         * 核心逻辑：解析请求URI → 拼接实际文件路径 → 读取文件内容 → 返回响应（404/200）
         */
        void file_handler(wsserver_t::connection_ptr &conn) 
        {
            //1.获取HTTP请求对象，提取请求URI（资源路径）
            websocketpp::http::parser::request req = conn->get_request();
            std::string uri = req.get_uri();

            //2.拼接文件实际路径（静态资源根目录 + URI）
            std::string realpath = _web_root + uri;

            //3.处理目录请求（默认返回login.html）
            // 例如：请求/ → 实际路径./wwwroot/login.html
            if (realpath.back() == '/') 
            {
                realpath += "login.html";
            }

            //4.读取文件内容
            Json::Value resp_json;
            std::string body;
            bool ret = file_util::read(realpath, body);

            //5.文件不存在 → 返回404响应
            if (ret == false) 
            {
                //5.1：构造404页面（UTF-8编码，避免中文乱码）
                body += "<html>";
                body += "<head>";
                body += "<meta charset='UTF-8'/>";
                body += "</head>";
                body += "<body>";
                body += "<h1> Not Found </h1>";
                body += "</body>";

                //5.2：设置HTTP状态码：404 Not Found
                conn->set_body(body);
                conn->set_status(websocketpp::http::status_code::not_found);
                return;
            }

            //6.文件存在 → 返回200响应，设置响应正文
            conn->set_body(body);
            conn->set_status(websocketpp::http::status_code::ok);
        }


        /**
         * @brief HTTP回调-用户注册请求处理（POST /reg）
         * @param conn 连接指针
         * @return void 无返回值
         * 核心逻辑：解析请求正文 → 校验参数 → 数据库插入用户 → 返回注册结果
         */
        void reg(wsserver_t::connection_ptr &conn) 
        {
            //1.获取HTTP请求对象，提取请求正文（JSON格式：{username, password}）
            std::string req_body = conn->get_request_body();

            // websocketpp::http::parser::request req = conn->get_request();
            // std::string req_body = rep->get_body();
            /*注意：上面的写法是错误的，因为：
            *    1. 这个 req 里面 只有请求头信息
            *    2. 请求方法（GET/POST）
            *    3. 请求路径（/reg/login）
            *    4. 请求头
            *  它不包含请求体 body！它没有 get_body () 这个函数！
            **/

            //2.反序列化请求正文，解析用户名/密码
            Json::Value login_info;
            bool ret = json_util::unserialize(req_body, login_info);
            if (ret == false) 
            {
                DLOG("反序列化注册信息失败");

                // 反序列化失败 → 返回400 Bad Request
                return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请求的正文格式错误");
            }

            //3.校验参数完整性（用户名/密码不能为空）
            if (login_info["username"].isNull() || login_info["password"].isNull()) 
            {
                DLOG("用户名密码不完整");
                return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请输入用户名/密码");
            }

            //4.数据库插入新用户（用户名唯一校验）
            ret = _ut.insert(login_info);
            if (ret == false) 
            {
                DLOG("向数据库插入数据失败");

                // 用户名已存在 → 返回400 Bad Request
                return http_resp(conn, false, websocketpp::http::status_code::bad_request, "用户名已经被占用!");
            }

            //5.注册成功 → 返回200 OK
            return http_resp(conn, true, websocketpp::http::status_code::ok, "注册用户成功");
        }

        /**
         * @brief HTTP回调-用户登录请求处理（POST /login）
         * @param conn 连接指针
         * @return void 无返回值
         * 核心逻辑：解析请求正文 → 校验用户名密码 → 创建Session → 设置Cookie → 返回登录结果
         */
        void login(wsserver_t::connection_ptr &conn) 
        {
            //1.解析请求正文（JSON格式：{username, password}）
            std::string req_body = conn->get_request_body();

            //2.反序列化请求正文，解析用户名/密码
            Json::Value login_info;
            bool ret = json_util::unserialize(req_body, login_info);
            if (ret == false) 
            {
                DLOG("反序列化登录信息失败");
                return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请求的正文格式错误");
            }

            //3.校验参数完整性
            if (login_info["username"].isNull() || login_info["password"].isNull()) 
            {
                DLOG("用户名密码不完整");
                return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请输入用户名/密码");
            }

            //4.数据库校验用户名密码（成功则login_info填充用户ID）
            ret = _ut.login(login_info);
            if (ret == false) 
            {
                DLOG("用户名密码错误");
                return http_resp(conn, false, websocketpp::http::status_code::bad_request, "用户名密码错误");
            }

            //5.创建Session（绑定用户ID，状态为LOGIN）
            uint64_t uid = login_info["id"].asUInt64();
            session_ptr ssp = _sm.create_session(uid, LOGIN);
            if (ssp.get() == nullptr) 
            {
                DLOG("创建会话失败");
                return http_resp(conn, false, websocketpp::http::status_code::internal_server_error , "创建会话失败");
            }

            //6.设置Session超时时间（30秒无操作自动销毁）
            _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);

            //7.设置Cookie（SSID=会话ID），返回给客户端
            std::string cookie_ssid = "SSID=" + std::to_string(ssp->ssid());
            conn->append_header("Set-Cookie", cookie_ssid);

            //8.登录成功 → 返回200 OK
            return http_resp(conn, true, websocketpp::http::status_code::ok , "登录成功");
        }

        /**
         * @brief Cookie解析工具函数（从Cookie字符串中提取指定Key的值）
         * @param cookie_str 完整的Cookie字符串（如：SSID=123; path=/;）
         * @param key 要提取的Key（如：SSID）
         * @param val 输出参数，存储提取到的Value
         * @return bool 提取成功返回true，失败返回false
         */
        bool get_cookie_val(const std::string &cookie_str, const std::string &key,  std::string &val) 
        {
            // Cookie格式示例：SSID=XXX; path=/; 
            //1.以"; "分割Cookie字符串，得到单个Cookie键值对
            std::string sep = "; ";
            std::vector<std::string> cookie_arr;
            string_util::split(cookie_str, sep, cookie_arr);

            //2.遍历单个Cookie，以"="分割Key/Value
            for (auto str : cookie_arr) 
            {
                //2.1：
                std::vector<std::string> tmp_arr;
                string_util::split(str, "=", tmp_arr);

                //2.2：格式错误（无=或多=），跳过
                if (tmp_arr.size() != 2) { continue; }
                
                //2.3：匹配目标Key，提取Value
                if (tmp_arr[0] == key) 
                {
                    val = tmp_arr[1];
                    return true;
                }
            }

            //3.未找到目标Key
            return false;
        }

        /**
         * @brief HTTP回调-用户信息查询请求处理（GET /info）
         * @param conn 连接指针
         * @return void 无返回值
         * 核心逻辑：解析Cookie → 校验Session → 查询用户信息 → 返回结果 + 刷新Session超时
         */
        void info(wsserver_t::connection_ptr &conn) 
        {
            //1.获取Cookie字符串（请求头Cookie）
            std::string cookie_str = conn->get_request_header("Cookie");
            if (cookie_str.empty()) 
            {
                // 无Cookie → 返回400，要求重新登录
                return http_resp(conn, true, websocketpp::http::status_code::bad_request, "找不到cookie信息，请重新登录");
            }

            //2.从Cookie中提取SSID（会话ID）
            std::string ssid_str;
            bool ret = get_cookie_val(cookie_str, "SSID", ssid_str);
            if (ret == false) 
            {
                // 无SSID → 返回400，要求重新登录
                return http_resp(conn, true, websocketpp::http::status_code::bad_request, "找不到ssid信息，请重新登录");
            }

            //3.通过SSID查询Session（校验登录状态）
            session_ptr ssp = _sm.get_session_by_ssid(std::stol(ssid_str));
            if (ssp.get() == nullptr) 
            {
                // Session过期/不存在 → 返回400，要求重新登录
                return http_resp(conn, true, websocketpp::http::status_code::bad_request, "登录过期，请重新登录");
            }

            //4.查询用户信息（通过Session绑定的用户ID）
            uint64_t uid = ssp->get_user();
            Json::Value user_info;
            ret = _ut.select_by_id(uid, user_info);
            if (ret == false) 
            {
                // 用户信息不存在 → 返回400
                return http_resp(conn, true, websocketpp::http::status_code::bad_request, "找不到用户信息，请重新登录");
            }

            //5.序列化用户信息，返回200响应
            std::string body;
            json_util::serialize(user_info, body);
            conn->set_body(body);
            conn->append_header("Content-Type", "application/json");
            conn->set_status(websocketpp::http::status_code::ok);

            //6.刷新Session超时时间（重置30秒倒计时）
            _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
        }

        /**
         * @brief HTTP请求分发器（核心回调）
         * @param hdl WebSocket++连接句柄
         * @return void 无返回值
         * 核心逻辑：根据请求方法+URI分发到对应处理函数
         *  1. POST /reg → 注册；2. POST /login → 登录；3. GET /info → 信息查询；4. 其他 → 静态资源
         */
        void http_callback(websocketpp::connection_hdl hdl) 
        {
            //1.获取连接指针（从句柄转换）
            wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);

            //2.提取请求方法（GET/POST）和URI
            websocketpp::http::parser::request req = conn->get_request();
            std::string method = req.get_method();
            std::string uri = req.get_uri();

            //3.分发请求
            if (method == "POST" && uri == "/reg") {
                return reg(conn);
            }else if (method == "POST" && uri == "/login") {
                return login(conn);
            }else if (method == "GET" && uri == "/info") {
                return info(conn);
            }else {
                return file_handler(conn);
            }
        }

        /**
         * @brief WebSocket响应封装工具函数（JSON格式响应）
         * @param conn WebSocket连接指针
         * @param resp JSON响应对象
         * @return void 无返回值
         */
        void ws_resp(wsserver_t::connection_ptr conn, Json::Value &resp) 
        {
            std::string body;
            json_util::serialize(resp, body);
            conn->send(body);
        }

        /**
         * @brief WebSocket通用工具函数-通过Cookie获取Session（带错误响应）
         * @param conn WebSocket连接指针
         * @return session_ptr 会话智能指针（失败返回空）
         * 核心逻辑：解析Cookie → 提取SSID → 查询Session → 失败则返回WebSocket错误响应
         */
        session_ptr get_session_by_cookie(wsserver_t::connection_ptr conn) 
        {
            //1.获取Cookie字符串
            Json::Value err_resp;
            std::string cookie_str = conn->get_request_header("Cookie");
            if (cookie_str.empty())
            {
                // 无Cookie → 返回错误响应
                err_resp["optype"] = "hall_ready";
                err_resp["reason"] = "没有找到cookie信息，需要重新登录";
                err_resp["result"] = false;
                ws_resp(conn, err_resp);
                return session_ptr();
            }

            //2.提取SSID
            std::string ssid_str;
            bool ret = get_cookie_val(cookie_str, "SSID", ssid_str);
            if (ret == false) 
            {
                // 无SSID → 返回错误响应
                err_resp["optype"] = "hall_ready";
                err_resp["reason"] = "没有找到SSID信息，需要重新登录";
                err_resp["result"] = false;
                ws_resp(conn, err_resp);
                return session_ptr();
            }

            //3.查询Session
            session_ptr ssp = _sm.get_session_by_ssid(std::stol(ssid_str));
            if (ssp.get() == nullptr) 
            {
                // Session过期 → 返回错误响应
                err_resp["optype"] = "hall_ready";
                err_resp["reason"] = "没有找到session信息，需要重新登录";
                err_resp["result"] = false;
                ws_resp(conn, err_resp);
                return session_ptr();
            }

            //4.成功获取Session
            return ssp;
        }

        /**
         * @brief WebSocket回调-游戏大厅连接建立处理（WS /hall）
         * @param conn WebSocket连接指针
         * @return void 无返回值
         * 核心逻辑：校验Session → 防重复登录 → 加入大厅 → 设置Session永久 → 返回成功响应
         */
        void wsopen_game_hall(wsserver_t::connection_ptr conn) 
        {
            //1.校验登录状态（获取Session）
            Json::Value resp_json;
            session_ptr ssp = get_session_by_cookie(conn);
            if (ssp.get() == nullptr) 
            {
                return;
            }

            //2.防重复登录（已在大厅/房间则拒绝）
            uint64_t uid = ssp->get_user();
            if (_om.is_in_game_hall(uid) || _om.is_in_game_room(uid)) 
            {
                resp_json["optype"] = "hall_ready";
                resp_json["reason"] = "玩家重复登录！";
                resp_json["result"] = false;
                return ws_resp(conn, resp_json);
            }

            //3.将用户+连接加入游戏大厅（在线管理器）
            _om.enter_game_hall(uid, conn);

            //4.返回大厅连接成功响应
            resp_json["optype"] = "hall_ready";
            resp_json["result"] = true;
            ws_resp(conn, resp_json);

            //5.设置Session永久存在（大厅内不超时）
            _sm.set_session_expire_time(ssp->ssid(), SESSION_FOREVER);
        }

        /**
         * @brief WebSocket回调-游戏房间连接建立处理（WS /room）
         * @param conn WebSocket连接指针
         * @return void 无返回值
         * 核心逻辑：校验Session → 防重复登录 → 校验房间 → 加入房间 → 设置Session永久 → 返回房间信息
         */
        void wsopen_game_room(wsserver_t::connection_ptr conn) 
        {
            //1.校验登录状态（获取Session）
            Json::Value resp_json;
            session_ptr ssp = get_session_by_cookie(conn);
            if (ssp.get() == nullptr) 
            {
                return;
            }

            //2.防重复登录
            uint64_t uid = ssp->get_user();
            if (_om.is_in_game_hall(uid) || _om.is_in_game_room(uid)) 
            {
                resp_json["optype"] = "room_ready";
                resp_json["reason"] = "玩家重复登录！";
                resp_json["result"] = false;
                return ws_resp(conn, resp_json);
            }

            //3.校验房间存在性（通过用户ID查询房间）
            room_ptr rp = _rm.get_room_by_uid(uid);
            if (rp.get() == nullptr) 
            {
                resp_json["optype"] = "room_ready";
                resp_json["reason"] = "没有找到玩家的房间信息";
                resp_json["result"] = false;
                return ws_resp(conn, resp_json);
            }

            //4.将用户+连接加入游戏房间（在线管理器）
            _om.enter_game_room(uid, conn);

            //5.设置Session永久存在（房间内不超时）
            _sm.set_session_expire_time(ssp->ssid(), SESSION_FOREVER);

            //6.返回房间连接成功响应（包含房间ID、玩家ID、黑白棋ID）
            resp_json["optype"] = "room_ready";
            resp_json["result"] = true;
            resp_json["room_id"] = (Json::UInt64)rp->id();
            resp_json["uid"] = (Json::UInt64)uid;
            resp_json["white_id"] = (Json::UInt64)rp->get_white_user();
            resp_json["black_id"] = (Json::UInt64)rp->get_black_user();
            return ws_resp(conn, resp_json);
        }

        /**
         * @brief WebSocket连接建立回调分发器（WS open）
         * @param hdl WebSocket连接句柄
         * @return void 无返回值
         * 核心逻辑：根据URI分发到大厅/房间连接处理函数
         */
        void wsopen_callback(websocketpp::connection_hdl hdl) 
        {
            //1.获取连接指针（从句柄转换）
            wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
            websocketpp::http::parser::request req = conn->get_request();

            //2.提取URI
            std::string uri = req.get_uri();

            //3.分发请求
            if (uri == "/hall") 
            {
                return wsopen_game_hall(conn);  // WS /hall → 游戏大厅连接
            }
            else if (uri == "/room") 
            {
                return wsopen_game_room(conn); // WS /room → 游戏房间连接
            }
        }

        /**
         * @brief WebSocket回调-游戏大厅连接断开处理
         * @param conn WebSocket连接指针
         * @return void 无返回值
         * 核心逻辑：校验Session → 退出大厅 → 恢复Session超时 → 清理资源
         */
        void wsclose_game_hall(wsserver_t::connection_ptr conn) 
        {
            //1.校验登录状态（获取Session）
            session_ptr ssp = get_session_by_cookie(conn);
            if (ssp.get() == nullptr) 
            {
                return;
            }

            //2.将玩家从游戏大厅移除（在线管理器）
            _om.exit_game_hall(ssp->get_user());

            //3.恢复Session超时管理（30秒无操作自动销毁）
            _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
        }

        /**
         * @brief WebSocket回调-游戏房间连接断开处理
         * @param conn WebSocket连接指针
         * @return void 无返回值
         * 核心逻辑：校验Session → 退出房间 → 恢复Session超时 → 清理房间用户
         */
        void wsclose_game_room(wsserver_t::connection_ptr conn)
        {
            //1.校验登录状态（获取Session）
            session_ptr ssp = get_session_by_cookie(conn);
            if (ssp.get() == nullptr) 
            {
                return;
            }

            //2.将玩家从游戏房间移除（在线管理器）
            _om.exit_game_room(ssp->get_user());

            //3.恢复Session超时管理
            _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);

            //4.移除房间内用户（房间无玩家则自动销毁）
            _rm.remove_room_user(ssp->get_user());
        }

        /**
         * @brief WebSocket连接断开回调分发器（WS close）
         * @param hdl WebSocket连接句柄
         * @return void 无返回值
         */
        void wsclose_callback(websocketpp::connection_hdl hdl) 
        {
            //1.获取连接指针（从句柄转换）
            wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
            websocketpp::http::parser::request req = conn->get_request();

            //2.提取URI
            std::string uri = req.get_uri();

            //3.分发请求
            if (uri == "/hall") 
            {
                return wsclose_game_hall(conn);
            }
            else if (uri == "/room") 
            {
                return wsclose_game_room(conn);
            }
        }

        /**
         * @brief WebSocket回调-游戏大厅消息处理（匹配开始/停止）
         * @param conn WebSocket连接指针
         * @param msg WebSocket消息指针（包含请求正文）
         * @return void 无返回值
         * 核心逻辑：校验Session → 解析消息 → 处理匹配开始/停止 → 返回结果
         */
        void wsmsg_game_hall(wsserver_t::connection_ptr conn, wsserver_t::message_ptr msg) 
        {
            //1.校验登录状态（获取Session）
            Json::Value resp_json;
            //std::string resp_body;
            session_ptr ssp = get_session_by_cookie(conn);
            if (ssp.get() == nullptr) 
            {
                return;
            }

            //2.解析WebSocket消息正文（JSON格式）
            Json::Value req_json;
            std::string req_body = msg->get_payload();
            bool ret = json_util::unserialize(req_body, req_json);
            if (ret == false) 
            {
                resp_json["result"] = false;
                resp_json["reason"] = "请求信息解析失败";
                return ws_resp(conn, resp_json);
            }

            //3.处理匹配请求
            std::string optype = req_json["optype"].asString();
            if (optype == "match_start")
            {
                // 开始匹配 → 添加到匹配队列
                _mm.add(ssp->get_user());
                resp_json["optype"] = "match_start";
                resp_json["result"] = true;
                return ws_resp(conn, resp_json);
            }
            else if (optype == "match_stop") 
            {
                // 停止匹配 → 从匹配队列移除
                _mm.del(ssp->get_user());
                resp_json["optype"] = "match_stop";
                resp_json["result"] = true;
                return ws_resp(conn, resp_json);
            }

            //4.未知请求类型 → 返回错误响应
            resp_json["optype"] = "unknow";
            resp_json["reason"] = "请求类型未知";
            resp_json["result"] = false;
            return ws_resp(conn, resp_json);
        }

        /**
         * @brief WebSocket回调-游戏房间消息处理（下棋/聊天）
         * @param conn WebSocket连接指针
         * @param msg WebSocket消息指针
         * @return void 无返回值
         * 核心逻辑：校验Session → 校验房间 → 解析消息 → 房间模块处理 → 自动广播结果
         */
        void wsmsg_game_room(wsserver_t::connection_ptr conn, wsserver_t::message_ptr msg) 
        {
            //1.校验登录状态（获取Session）
            Json::Value resp_json;
            session_ptr ssp = get_session_by_cookie(conn);
            if (ssp.get() == nullptr) 
            {
                DLOG("房间-没有找到会话信息");
                return;
            }

            //2.校验房间存在性
            room_ptr rp = _rm.get_room_by_uid(ssp->get_user());
            if (rp.get() == nullptr) 
            {
                resp_json["optype"] = "unknow";
                resp_json["reason"] = "没有找到玩家的房间信息";
                resp_json["result"] = false;
                DLOG("房间-没有找到玩家房间信息");
                return ws_resp(conn, resp_json);
            }

            //3.解析WebSocket消息正文
            Json::Value req_json;
            std::string req_body = msg->get_payload();
            bool ret = json_util::unserialize(req_body, req_json);
            if (ret == false) 
            {
                resp_json["optype"] = "unknow";
                resp_json["reason"] = "请求解析失败";
                resp_json["result"] = false;
                DLOG("房间-反序列化请求失败");
                return ws_resp(conn, resp_json);
            }
            DLOG("房间：收到房间请求，开始处理....");

            //4.交给房间模块处理（自动广播结果）
            return rp->handle_request(req_json);
        }

        /**
         * @brief WebSocket消息回调分发器（WS message）
         * @param hdl WebSocket连接句柄
         * @param msg WebSocket消息指针
         * @return void 无返回值
         */
        void wsmsg_callback(websocketpp::connection_hdl hdl, wsserver_t::message_ptr msg) 
        {
            //1.获取连接指针（从句柄转换）
            wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
            websocketpp::http::parser::request req = conn->get_request();

            //2.提取URI
            std::string uri = req.get_uri();

            //2.分发请求
            if (uri == "/hall") {
                return wsmsg_game_hall(conn, msg);
            }
            else if (uri == "/room") {
                return wsmsg_game_room(conn, msg);
            }
        }

    public:
        /**
         * @brief 构造函数：初始化服务器核心模块，设置回调函数
         * @param host 数据库主机地址
         * @param user 数据库用户名
         * @param pass 数据库密码
         * @param dbname 数据库名
         * @param port 数据库端口（默认3306）
         * @param wwwroot 静态资源根目录（默认./wwwroot/）
         */
        gobang_server(const std::string &host,
               const std::string &user,
               const std::string &pass,
               const std::string &dbname,
               uint16_t port = 3306,
               const std::string &wwwroot = WWWROOT):
               _web_root(wwwroot), _ut(host, user, pass, dbname, port),
               _rm(&_ut, &_om), _sm(&_wssrv), _mm(&_rm, &_ut, &_om) 
        {
            //1.关闭WebSocket++日志（减少冗余输出）
            _wssrv.set_access_channels(websocketpp::log::alevel::none);

            //2.初始化ASIO（网络IO库）
            _wssrv.init_asio();

            //3.设置端口复用（避免服务器重启后端口占用）
            _wssrv.set_reuse_addr(true);

            //4.绑定回调函数
            _wssrv.set_http_handler(std::bind(&gobang_server::http_callback, this, std::placeholders::_1));
            _wssrv.set_open_handler(std::bind(&gobang_server::wsopen_callback, this, std::placeholders::_1));
            _wssrv.set_close_handler(std::bind(&gobang_server::wsclose_callback, this, std::placeholders::_1));
            _wssrv.set_message_handler(std::bind(&gobang_server::wsmsg_callback, this, std::placeholders::_1, 
                std::placeholders::_2));
        }

        /**
         * @brief 启动服务器（监听端口，运行事件循环）
         * @param port 服务器监听端口（如8080）
         * @return void 无返回值
         */
        void start(int port) 
        {
            _wssrv.listen(port);    // 监听指定端口
            _wssrv.start_accept();  // 开始接受连接
            _wssrv.run();           // 运行ASIO事件循环（阻塞）
        }
};

// 结束头文件保护宏
#endif