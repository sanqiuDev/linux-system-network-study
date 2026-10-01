#include "cpp-httplib/httplib.h"  // cpp-httplib轻量级HTTP服务器库（提供HTTP服务核心能力）
#include "searcher.hpp"           // 搜索核心模块（处理分词、索引查询、结果构建）

// ===================== 全局常量定义 =====================
//1.预处理后的文档数据文件路径（索引构建的数据源）
const std::string input = "data/raw_html/raw.txt";

//2.HTTP服务器的静态资源根目录（存放前端页面、CSS、JS等）
const std::string root_path = "./wwwroot";

// ===================== 主函数（HTTP服务器入口） =====================
/**
 * @brief 程序入口函数，启动HTTP服务器并处理搜索请求
 * @details 核心流程：
 *          1. 初始化搜索器（加载/构建正倒排索引）
 *          2. 配置HTTP服务器（静态资源目录、路由规则）
 *          3. 启动服务器，监听8081端口（对外提供搜索接口）
 * @return int 程序退出码（0为正常退出，异常时服务器会阻塞）
 */
int main()
{
    // -------- 步骤1：初始化搜索器（加载索引） --------
    //1.创建搜索器实例
    ns_searcher::Searcher search; 
    //2.初始化搜索器：基于预处理后的文档数据构建正倒排索引
    search.InitSearcher(input);

    // -------- 步骤2：创建并配置HTTP服务器 --------
    //3.创建cpp-httplib服务器实例
    httplib::Server svr; 

    //4.设置静态资源根目录（前端页面、CSS、JS等文件的存放路径）
    svr.set_base_dir(root_path.c_str()); // 浏览器访问根路径（http://ip:8081/）时，会自动加载该目录下的index.html 

    // -------- 步骤3：注册搜索接口路由（GET /s） --------
    /**
     * @brief 注册GET请求路由：/s（搜索接口）
     * @param "/s" 路由路径（前端通过http://ip:8081/s?word=关键词 访问）
     * @param 匿名lambda函数：处理搜索请求并生成响应
     *        req：请求对象（包含请求参数、头部等）
     *        rsp：响应对象（用于设置响应内容、状态码、头部等）
     */
    //5.
    svr.Get("/s", [&search](const httplib::Request &req, httplib::Response &rsp)
        {
            //5.1：校验请求参数：必须包含"word"参数（搜索关键词）
            if(!req.has_param("word"))
            {
                // 参数缺失：返回提示信息，设置响应类型为纯文本（UTF-8编码避免中文乱码）
                rsp.set_content("必须要有搜索关键字!", "text/plain; charset=utf-8");

                // 终止当前请求处理
                return; 
            }

            //5.2：提取搜索关键词（从请求参数中获取"word"的值）
            std::string word = req.get_param_value("word");
            LOG(NORMAL, "用户搜索的: " + word); // 记录日志：打印用户搜索的关键词

            //5.3：调用搜索核心逻辑，生成JSON格式的搜索结果
            std::string json_string;           // 存储JSON格式的搜索结果
            search.Search(word, &json_string); // 处理搜索请求，生成JSON

            //5.4：构建HTTP响应：返回JSON数据，设置响应类型为application/json
            rsp.set_content(json_string, "application/json");
        }
    );

    // -------- 步骤4：启动HTTP服务器 --------
    LOG(NORMAL, "服务器启动成功..."); // 打印启动日志

    //6.
    /**
     * @brief 启动服务器，监听指定地址和端口
     * @param "0.0.0.0"：监听所有网络接口（允许外网/内网访问）
     * @param 8081：监听的端口号
     * @note listen是阻塞调用，服务器启动后会一直运行，直到手动终止（Ctrl+C）
     */
    svr.listen("0.0.0.0", 8081);

    return 0; 
}
