#pragma once  


/*--------------------------------------------头文件--------------------------------------------*/
#include <iostream>          // 标准输入输出 ---> 调试打印
#include <string>            // 字符串处理 ---> HTTP报文解析
#include <memory>            // 智能指针 ---> 管理对象生命周期
#include <sstream>           // 字符串流 ---> 解析HTTP请求行
#include <functional>        // 函数包装器 ---> 定义HTTP处理回调
#include <unordered_map>     // 哈希表 ---> 存储HTTP头部、路由映射

#include "Socket.hpp"        // Socket模块 ---> 提供网络通信能力
#include "TcpServer.hpp"     // TCP服务器类 ---> 封装监听和连接管理
#include "Log.hpp"           // 日志模块 ---> 记录调试、警告等信息
#include "Util.hpp"          // 工具类 ---> 提供文件读取、字符串处理等辅助功能


/*--------------------------------------------命名空间 + 常量定义 + 目录配置--------------------------------------------*/
//1.引入命名空间
using namespace SocketModule;  // 引入Socket模块命名空间
using namespace LogModule;     // 引入日志模块命名空间

//2.定义HTTP协议常量（分隔符、路径等）
const std::string gspace = " ";       // 空格分隔符 ---> 用于请求行、状态行
const std::string glinespace = "\r\n";// 行分隔符 ---> HTTP头部每行以\r\n结束
const std::string glinesep = ": ";    // 头部键值分隔符 ---> 如"Content-Length: 1024"

//3.网站根目录及默认页面配置
const std::string webroot = "./wwwroot";   // 静态资源根目录 ---> 存放HTML、图片等
const std::string homepage = "index.html"; // 默认首页 ---> 访问时返回
const std::string page_404 = "/404.html";  // 404错误页面


/*--------------------------------------------【HTTP请求类】--------------------------------------------*/
// 解析客户端发送的HTTP请求报文
class HttpRequest
{
private:
    //1.请求方法（如：GET、POST）
    //2.统一资源标识符（转换为本地路径）
    //3.HTTP版本（如：HTTP/1.1）
    std::string _method;  
    std::string _uri;     
    std::string _version;

    //4.请求头部（键值对）
    //5.空行（头部与正文的分隔）
    //6.请求正文（POST方法的参数通常在这里）
    std::unordered_map<std::string, std::string> _headers; 
    std::string _blankline;                               
    std::string _text;                                     

    //7.请求参数（GET方法的参数，如：?a=1&b=2）
    //8.是否为交互请求（true=带参数，需动态处理）
    std::string _args; 
    bool _is_interact; 

public:
    /*--------------------------------------<构造&析构>--------------------------------------*/
    //1.“构造函数” ---> 初始化交互标记为false（默认是静态资源请求）
    HttpRequest() : _is_interact(false) {}

    //2.“析构函数”
    ~HttpRequest() {}


    /*--------------------------------------<序列&反序列>--------------------------------------*/
    //1.“解析请求行” ---> 如"GET /index.html HTTP/1.1"
    void ParseReqLine(std::string &reqline)
    {
        //1.用字符串流分割请求行
        std::stringstream ss(reqline);  

        //2.提取方法、URI、版本
        ss >> _method >> _uri >> _version;
    }

    //2.“序列化” ---> （未实现，因为客户端将会使用浏览器向服务端发送HTTP请求）
    std::string Serialize()
    {
        return std::string();
    }

    //3.“反序列化” ---> 将HTTP请求字符串解析为HttpRequest对象
    bool Deserialize(std::string &reqstr)
    {
        //1.提取请求行（第一行数据）
        std::string reqline;
        bool res = Util::ReadOneLine(reqstr, &reqline, glinespace);  // 读取一行（以\r\n结束）
        LOG(LogLevel::DEBUG) << reqline; 

        //2.解析请求行
        ParseReqLine(reqline);

        //3.处理无参数的URI ---> 转换为本地文件路径
        //情况一：访问根路径时，默认返回首页
        if (_uri == "/")  
        {
            _uri = webroot + _uri + homepage;  // ./wwwroot/index.html
        }
        //情况二：其他路径直接拼接根目录
        else  
        {
            _uri = webroot + _uri;            // ./wwwroot/login.html
        }

        //4.处理带参数的URI（如：/login?username=xxx&password=xxx）
        const std::string temp = "?";
        auto pos = _uri.find(temp);  

        //情况一：无参数，是静态资源请求
        if (pos == std::string::npos)  
        {
            return true;
        }
        //情况二：有参数，标记为交互请求（动态处理）
        else
        {
            //1)提取参数部分（username=xxx&...）
            _args = _uri.substr(pos + temp.size());

            //2)截取URI路径部分（如：./wwwroot/login）
            _uri = _uri.substr(0, pos);          

            //3)标记为交互请求
            _is_interact = true;                   

            return true;
        }
    }


    /*--------------------------------------<get方法>--------------------------------------*/
    //1.获取解析后的本地文件路径
    std::string Uri() { return _uri; }          

    //2.判断是否为带参数的交互请求
    bool isInteract() { return _is_interact; }  

    //3.获取请求参数
    std::string Args() { return _args; }       
};



/*--------------------------------------------【HTTP响应类】--------------------------------------------*/
// 构建服务器返回的HTTP响应报文
class HttpResponse
{
public:
    //1.HTTP版本（如HTTP/1.0）
    //2.状态码（如200、404）
    //3.状态描述（如OK、Not Found）
    std::string _version; 
    int _code;            
    std::string _desc;    

    //4.响应头部
    //5.空行（头部与正文的分隔）
    //6.响应正文（文件内容或动态生成的HTML）
    std::unordered_map<std::string, std::string> _headers; 
    std::string _blankline;                                
    std::string _text;                                    

    //7.目标文件路径（客户端请求的资源）
    std::string _targetfile; 

public:
    /*--------------------------------------<构造&析构>--------------------------------------*/
    //1.“构造函数” ---> 初始化HTTP版本和空行分隔符
    HttpResponse() : _blankline(glinespace), _version("HTTP/1.0"){ }

    //2.“析构函数”
    ~HttpResponse() {}

    /*--------------------------------------<序列&反序列>--------------------------------------*/
    //1.“序列化” ---> 将响应对象转换为HTTP响应字符串（用于网络传输）
    std::string Serialize()
    {
        //1.构建状态行（如："HTTP/1.0 200 OK\r\n"）
        std::string status_line = _version + gspace + std::to_string(_code) + gspace + _desc + glinespace;

        //2.构建响应头部（键值对拼接）
        std::string resp_header;
        for (auto &header : _headers)
        {
            std::string line = header.first + glinesep + header.second + glinespace; // 头部格式："Key: Value\r\n"
            resp_header += line;
        }

        //3.拼接完整响应（状态行 + 响应头部 + 空行 + 正文）
        return status_line + resp_header + _blankline + _text;
    }

    //2.“反序列化”（未实现，因为客户端将会使用浏览器进行访问服务端）
    bool Deserialize(std::string &reqstr)
    {
        return true;
    }

    /*--------------------------------------<设置>--------------------------------------*/
    //1.“设置响应状态码及描述” 
    void SetCode(int code)
    {
        _code = code;

        switch (_code)
        {
        case 200:
            _desc = "OK";        // 成功
            break;
        case 404:
            _desc = "Not Found"; // 资源不存在
            break;
        case 301:
            _desc = "Moved Permanently"; // 永久重定向
            break;
        case 302:
            _desc = "See Other";         // 临时重定向
            break;
        default:
            break;
        }
    }


    //2.“添加响应头部” ---> 如："Content-Type: text/html"
    void SetHeader(const std::string &key, const std::string &value)
    {
        auto iter = _headers.find(key);
        if (iter != _headers.end())
        {
            return;  // 已存在则不重复添加
        }
        _headers.insert(std::make_pair(key, value));
    }

    
    //3.“设置响应正文” ---> 用于动态生成内容
    void SetText(const std::string &t)
    {
        _text = t;
    }


    //4.“设置目标文件” ---> 客户端请求的资源路径
    void SetTargetFile(const std::string &target)
    {
        _targetfile = target;
    }

    
    /*--------------------------------------<获取>--------------------------------------*/
    //1.“根据文件路径获取MIME类型” ---> 用于Content-Type头部
    std::string Uri2Suffix(const std::string &targetfile)
    {
        // 查找最后一个.（获取文件后缀）
        auto pos = targetfile.rfind(".");
        if (pos == std::string::npos)
        {
            return "text/html";  // 无后缀默认视为HTML
        }

        std::string suffix = targetfile.substr(pos);  // 提取后缀（如.html）
        // 映射常见后缀到MIME类型
        if (suffix == ".html" || suffix == ".htm")
            return "text/html";
        else if (suffix == ".jpg" || suffix == ".jpeg")
            return "image/jpeg";
        else if (suffix == ".png")
            return "image/png";
        else
            return "";  // 未知类型
    }


    /*--------------------------------------<构建>--------------------------------------*/
    //1.“构建HTTP响应内容” ---> 核心逻辑是根据请求的目标文件，返回对应静态资源、重定向响应或404错误页面
    bool MakeResponse()
    {
        //1.忽略浏览器默认发送的/favicon.ico请求（网页图标）
        if (_targetfile == "./wwwroot/favicon.ico") 
        {
            LOG(LogLevel::DEBUG) << "用户请求: " << _targetfile << "忽略它";
            return false; // 浏览器访问网页时会自动请求该图标，服务器无需处理，减少无效日志和响应
        }

        //2.测试重定向功能 ---> 当请求/redir_test路径时，返回301永久重定向响应
        if (_targetfile == "./wwwroot/redir_test")
        {
            //2.1：设置响应状态码为301（Moved Permanently，永久重定向）
            SetCode(301); 

            //2.2：设置Location响应头，指定重定向的目标地址（腾讯首页）
            SetHeader("Location", "https://www.qq.com/");

            //2.3：返回true，告知上层需要发送重定向响应
            return true; 
        }

        //3.读取客户端请求的目标文件，构建正常响应或404错误响应
        //3.1：存储文件大小，用于设置Content-Length响应头
        int filesize = 0; 

        //3.2：调用工具类读取目标文件内容到响应正文_text中（二进制模式，支持图片/HTML等所有文件）
        bool res = Util::ReadFileContent(_targetfile, &_text);

        //3.3：
        //情况一：文件读取失败，返回404 Not Found错误页面
        if (!res)
        {
            //1）清空原有正文，避免脏数据
            _text = ""; 
            //2）记录警告日志，便于排查客户端访问的不存在资源
            LOG(LogLevel::WARNING) << "client want get : " << _targetfile << " but not found";

            //3）设置状态码为404（Not Found，资源不存在）
            SetCode(404);                     

            //4）将目标文件切换为404错误页面路径（./wwwroot/404.html）
            _targetfile = webroot + page_404;
            //5）读取404页面的大小和内容，填充到响应中
            filesize = Util::FileSize(_targetfile);     // 获取404页面的字节大小
            Util::ReadFileContent(_targetfile, &_text); // 读取404页面内容到响应正文

            //6）根据404页面的后缀（如：.html）获取对应的MIME类型（如：text/html）
            std::string suffix = Uri2Suffix(_targetfile);
            //7）设置Content-Type头，告知浏览器响应内容的类型
            SetHeader("Content-Type", suffix); 

            //8）设置Content-Length头，告知浏览器响应正文的字节数（解决TCP粘包/半包问题）
            SetHeader("Content-Length", std::to_string(filesize));
        }

        //情况二：文件读取成功，返回正常的200响应
        else
        {
            //1）记录调试日志，确认成功读取的文件路径
            LOG(LogLevel::DEBUG) << "读取文件: " << _targetfile;

            //2）设置状态码为200（OK，请求成功）
            SetCode(200); 

            //3）获取目标文件的大小，用于设置Content-Length响应头
            filesize = Util::FileSize(_targetfile);
            //4）设置Content-Length，确保浏览器能完整接收响应正文
            SetHeader("Content-Length", std::to_string(filesize));

            //5）根据文件后缀（如：.html/.jpg/.png）获取对应的MIME类型
            std::string suffix = Uri2Suffix(_targetfile);
            //6）设置Content-Type，浏览器据此解析内容（如渲染HTML、显示图片）
            SetHeader("Content-Type", suffix); 
        }

        //4.返回true，告知上层需要发送构建好的响应
        return true; 
    }
};




// 定义HTTP处理回调函数类型 ---> 处理交互请求（动态内容）
using http_func_t = std::function<void(HttpRequest &req, HttpResponse &resp)>;

/*--------------------------------------------【HTTP服务器类】--------------------------------------------*/
// 整合TCP服务器与HTTP协议处理，支持静态资源和动态交互
class Http
{
private:
    //1.定义TCP服务器对象
    //2.定义路由映射对象
    std::unique_ptr<TcpServer> tsvrp; 
    std::unordered_map<std::string, http_func_t> _route; //本地文件路径 -> 处理函数（用于动态交互请求）

public:
    //1.“构造函数” ---> 初始化TCP服务器（指定监听端口）
    Http(uint16_t port) : tsvrp(std::make_unique<TcpServer>(port)){}

    //2.“析构函数” ---> 初始化TCP服务器（指定监听端口）
    ~Http() {}

    //3.“处理HTTP请求” ---> TCP连接的回调函数
    void HandlerHttpRquest(std::shared_ptr<Socket> &sock, InetAddr &client)
    {
        //1.读取HTTP请求报文
        std::string httpreqstr;

        //2.从Socket读取数据
        int n = sock->Recv(&httpreqstr);  
        if (n > 0)  
        {
            //2.1：打印请求报文
            std::cout << "##########################" << std::endl;
            std::cout << httpreqstr;
            std::cout << "##########################" << std::endl;

            //2.2：定义请求和响音对象
            HttpRequest req;  
            HttpResponse resp;  

            //2.3：解析请求 ---> 对读取HTTP请求报文进行反序列化
            req.Deserialize(httpreqstr);  

            //2.4：带参数的交互请求（动态处理）
            if (req.isInteract())  
            {
                //1）路由映射中有对应的处理函数
                if (_route.find(req.Uri()) == _route.end())
                {
                    // 路由未注册（可返回404或重定向）
                }

                //情况二：路由映射中没有对应的处理函数
                else
                {
                    //1）调用注册的处理函数处理请求
                    _route[req.Uri()](req, resp);

                    //2）序列化响应并发送
                    std::string response_str = resp.Serialize();
                    sock->Send(response_str);
                }
            }
            //2.5：静态资源请求（直接返回文件）
            else  
            {
                //1）设置目标文件
                resp.SetTargetFile(req.Uri()); 

                //2）构建响应内容
                if (resp.MakeResponse()) 
                {
                    std::string response_str = resp.Serialize();  // 序列化响应
                    sock->Send(response_str);                     // 发送响应
                }
            }
        }

        //3.调试模式代码（仅编译时生效，用于快速测试）
#ifdef DEBUG
        // 读取请求并返回默认首页
        std::string httpreqstr;
        sock->Recv(&httpreqstr);
        std::cout << httpreqstr;

        HttpResponse resp;
        resp._version = "HTTP/1.1";
        resp._code = 200;
        resp._desc = "OK";

        std::string filename = webroot + homepage;
        bool res = Util::ReadFileContent(filename, &(resp._text));
        (void)res;  // 忽略返回值
        std::string response_str = resp.Serialize();
        sock->Send(response_str);
#endif
    }


    //2.“启动HTTP服务器”
    void Start()
    {
        // 启动TCP服务器，设置连接处理回调为当前类的HandlerHttpRquest方法
        tsvrp->Start([this](std::shared_ptr<Socket> &sock, InetAddr &client)
                     { this->HandlerHttpRquest(sock, client); });
    }

    //3.“注册动态路由” ---> 将URI路径与处理函数绑定
    // 参数：name - 相对路径（如"/login"）；h - 处理该路径的回调函数
    void RegisterService(const std::string name, http_func_t h)
    {
        std::string key = webroot + name;  // 转换为本地路径（如./wwwroot/login）
        auto iter = _route.find(key);
        if (iter == _route.end())  // 未注册则添加
        {
            _route.insert(std::make_pair(key, h));
        }
    }
};