#pragma once  

#include <algorithm>                // 算法库（sort排序、search查找）
#include <unordered_map>            // 哈希表（临时存储文档权重）
#include <jsoncpp/json/json.h>      // JsonCpp库（序列化搜索结果为JSON）

#include "index.hpp"                // 索引模块（正倒排索引定义）
#include "util.hpp"                 // 工具模块（分词、字符串处理）
#include "log.hpp"                  // 日志模块（打印运行日志）


// ===================== 搜索模块命名空间 =====================
/**
 * @brief 搜索核心模块命名空间，封装搜索引擎的查询响应逻辑：
 *        1. 搜索初始化（加载索引）
 *        2. 搜索处理（分词→触发→排序→结果构建）
 *        3. 摘要生成（从文档内容中提取关键词上下文）
 */
namespace ns_searcher
{

    // ===================== 数据结构定义 =====================
    /**
     * @brief 倒排元素扩展结构体（用于搜索结果合并）
     * @details 相比原始InvertedElem，新增：
     *          1. 多关键词的总权重（单个文档的所有关键词权重之和）
     *          2. 匹配的关键词列表（方便后续生成摘要）
     */
    struct InvertedElemPrint
    {
        uint64_t doc_id;                // 文档ID
        int weight;                     // 该文档的总相关性权重（所有匹配关键词的权重和）
        std::vector<std::string> words; // 该文档匹配的关键词列表
        
        // 构造函数，初始化权重和文档ID为0
        InvertedElemPrint():doc_id(0), weight(0){}
    };

    // ===================== 搜索核心类 =====================
    /**
     * @brief 搜索核心类，处理用户查询请求并返回结构化结果
     * @details 核心流程：
     *          1. 初始化：加载/构建索引
     *          2. 搜索：分词→触发索引→合并权重→排序→生成JSON结果
     */
    class Searcher
    {
        private:
            //1.索引实例指针（指向全局唯一的正倒排索引）
            ns_index::Index *index; 

        public:
            //2.构造函数（空实现，仅初始化索引指针为nullptr）
            Searcher(){}
            
            //3.析构函数（空实现，索引实例由单例管理，无需手动释放）
            ~Searcher(){}

        public:
            /**
             * @brief 搜索器初始化（加载/构建索引）
             * @param input 输入参数：预处理后的文档数据文件路径（如data/raw_html/raw.txt）
             * @details 步骤：
             *          1. 获取索引单例实例
             *          2. 基于文档数据构建正倒排索引
             */
            void InitSearcher(const std::string &input)
            {
                //1.获取索引单例（全局唯一）
                index = ns_index::Index::GetInstance();
                LOG(NORMAL, "获取index单例成功...");

                //2.构建正倒排索引（从预处理文件加载文档并生成索引）
                index->BuildIndex(input);
                LOG(NORMAL, "建立正排和倒排索引成功...");
            }

            /**
             * @brief 处理用户搜索请求，生成JSON格式的搜索结果
             * @param query 输入参数：用户的搜索关键词（如："Boost 字符串处理"）
             * @param json_string 输出参数：存储JSON格式搜索结果的字符串指针
             * @details 核心流程：
             *          1. 分词：对搜索关键词进行中文分词
             *          2. 触发：查询每个分词的倒排索引，收集匹配文档
             *          3. 合并：按文档ID合并权重（多关键词累加）
             *          4. 排序：按总权重降序排序（相关性从高到低）
             *          5. 构建：生成JSON格式的结果（标题、摘要、URL等）
             */
            void Search(const std::string &query, std::string *json_string)
            {
                // -------- 步骤1：对搜索关键词进行分词 --------
                //1.存储分词结果
                std::vector<std::string> words; 

                //2.调用结巴分词（自动过滤停用词）
                ns_util::JiebaUtil::CutString(query, &words); 

                // -------- 步骤2：触发倒排索引，收集匹配文档 --------
                //3.哈希表：key=文档ID，value=该文档的权重和+匹配关键词（临时存储，用于合并）
                std::unordered_map<uint64_t, InvertedElemPrint> tokens_map;

                //4.遍历每个分词后的关键词
                for(std::string word : words)
                {
                    //4.1：转为小写（与索引构建时的规则一致，避免大小写敏感）
                    boost::to_lower(word); 

                    //4.2：查询该关键词的倒排拉链
                    ns_index::InvertedList *inverted_list = index->GetInvertedList(word);
                    if(inverted_list == nullptr){
                        continue;
                    }

                    //4.3：遍历该关键词的倒排拉链，合并到tokens_map
                    for(const auto &elem : *inverted_list)
                    {
                        // 按文档ID查找/创建条目（[]：存在则获取，不存在则新建）
                        auto &item = tokens_map[elem.doc_id];

                        item.doc_id = elem.doc_id;          // 填充文档ID
                        item.weight += elem.weight;         // 累加权重（多关键词权重和）
                        item.words.push_back(elem.word);    // 记录匹配的关键词
                    }
                }

                // -------- 步骤3：将哈希表结果转为vector，方便排序 --------
                //5.
                std::vector<InvertedElemPrint> inverted_list_all;
                for(const auto &item : tokens_map){ 
                    inverted_list_all.push_back(std::move(item.second)); // 移动语义：减少拷贝（item.second是局部临时对象）
                }

                // -------- 步骤4：按相关性权重降序排序 --------
                //6.
                std::sort(inverted_list_all.begin(), inverted_list_all.end(),
                        [](const InvertedElemPrint &e1, const InvertedElemPrint &e2){
                            return e1.weight > e2.weight;
                        });

                // -------- 步骤5：构建JSON格式的搜索结果 --------
                //7.JSON根节点（数组类型，存储所有搜索结果）
                Json::Value root; 

                //8.遍历排序后的结果，逐个构建JSON元素
                for(auto &item : inverted_list_all)
                {
                    //8.1：根据文档ID查询正排索引，获取文档完整信息
                    ns_index::DocInfo * doc = index->GetForwardIndex(item.doc_id);
                    if(doc == nullptr){
                        continue;
                    }

                    //8.2：构建单个搜索结果的JSON节点
                    Json::Value elem;
                    elem["title"] = doc->title;                          // 文档标题
                    elem["desc"] = GetDesc(doc->content, item.words[0]); // 文档摘要（取第一个匹配关键词的上下文）
                    elem["url"]  = doc->url;                             // 文档URL
                    elem["id"] = (int)item.doc_id;                       // 文档ID（调试用）
                    elem["weight"] = item.weight;                        // 相关性权重（调试用）

                    //8.3：将单个结果添加到根数组
                    root.append(elem); 
                }

                // -------- 步骤6：序列化JSON为字符串 --------
                // Json::FastWriter：快速序列化（无格式化，体积小，适合网络传输）
                // Json::StyledWriter：格式化序列化（易读，适合调试）
                Json::FastWriter writer;
                *json_string = writer.write(root); // 将JSON对象转为字符串
            }

            /**
             * @brief 生成文档摘要（从内容中提取关键词的上下文）
             * @param html_content 输入参数：文档的纯文本内容（已去标签）
             * @param word 输入参数：匹配的关键词（用于定位上下文）
             * @return std::string 生成的摘要字符串（关键词前后各50/100字符，末尾加...）
             * @details 逻辑：
             *          1. 查找关键词在内容中的首次出现位置（忽略大小写）
             *          2. 往前取50字符（不足则从开头），往后取100字符（不足则到结尾）
             *          3. 截取该段内容作为摘要，末尾加...
             */
            std::string GetDesc(const std::string &html_content, const std::string &word)
            {
                //1.摘要截取规则：关键词前50字符，后100字符
                const int prev_step = 50;  // 关键词前截取长度
                const int next_step = 100; // 关键词后截取长度

                //2.查找关键词在内容中的首次出现位置（忽略大小写）
                auto iter = std::search(
                    html_content.begin(), html_content.end(), // 待查找的内容范围
                    word.begin(), word.end(),                 // 要查找的关键词
                    [](int x, int y){                         // 自定义匹配规则：忽略大小写
                        return (std::tolower(x) == std::tolower(y));
                    }
                );

                //3.处理关键词未出现在内容中的情况
                if(iter == html_content.end()){ 
                    return "None1";
                }

                //4.计算关键词首次出现的位置（距离内容开头的偏移量）
                int pos = std::distance(html_content.begin(), iter);

                //5.确定摘要的起始和结束位置 
                int start = 0;                        // 摘要起始位置（默认开头）
                int end = html_content.size() - 1;    // 摘要结束位置（默认结尾）

                //6.如果关键词前有足够字符，起始位置后移prev_step
                if(pos > start + prev_step) start = pos - prev_step;
                //7.如果关键词后有足够字符，结束位置前移next_step
                if(pos < end - next_step) end = pos + next_step;

                //8.处理边界异常（如内容过短）的情况
                if(start >= end){
                    return "None2"; 
                } 

                //9.截取[start, end)范围内的子串作为摘要
                std::string desc = html_content.substr(start, end - start);
                desc += "..."; // 摘要末尾加省略号，标识内容被截断
                return desc;
            }
    };
}

