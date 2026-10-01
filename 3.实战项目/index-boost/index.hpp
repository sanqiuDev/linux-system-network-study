#pragma once 

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <mutex>

#include "util.hpp"       // 自定义工具库（字符串切割、分词等）
#include "log.hpp"        // 自定义日志库（打印运行日志）

// ===================== 命名空间定义 =====================
/**
 * @brief 索引模块命名空间，封装正倒排索引的核心逻辑
 * @details 该命名空间包含：
 *          1. 文档信息结构体（正排索引单元）
 *          2. 倒排元素结构体（倒排索引单元）
 *          3. 索引核心类（单例模式，负责构建/查询正倒排索引）
 */
namespace ns_index
{
    // ===================== 数据结构定义 =====================
    /**
     * @brief 正排索引结构体，存储单个文档的完整信息
     * @details 正排索引以文档ID为键，映射到文档的元数据，是搜索引擎的基础数据单元
     */
    struct DocInfo
    {
        std::string title;    // 文档标题（从HTML<title>标签提取）
        std::string content;  // 文档纯文本内容（去除所有HTML标签后的内容）
        std::string url;      // 文档对应的Boost官网URL

        uint64_t doc_id;      // 文档唯一ID（正排索引数组的下标，天然唯一）
    };

    /**
     * @brief 倒排索引元素结构体，存储单个关键词在某文档中的匹配信息
     * @details 是倒排拉链的基本单元，关联关键词、文档ID和相关性权重
     */
    struct InvertedElem
    {
        uint64_t doc_id;  // 关键词所在文档的ID
        std::string word; // 关键词本身（统一小写）
        int weight;       // 关键词在该文档中的权重（相关性分数）
        
        //构造函数，初始化权重为0
        InvertedElem():weight(0){}
    };

    /**
     * @brief 倒排拉链类型别名，存储同一关键词在所有文档中的匹配信息
     * @details 一个关键词对应一个倒排拉链，拉链中的元素按权重排序（后续扩展）
     */
    typedef std::vector<InvertedElem> InvertedList;

    // ===================== 索引核心类 =====================
    /**
     * @brief 索引核心类，实现正倒排索引的构建与查询
     * @details 采用单例模式（保证全局唯一索引实例），包含：
     *          1. 正排索引：文档ID → 文档完整信息
     *          2. 倒排索引：关键词 → 倒排拉链（该关键词在所有文档中的匹配信息）
     */
    class Index
    {
        private:
            // -------- 核心数据结构 --------
            /**
             * @brief 正排索引：vector数组存储所有文档信息
             * @details 数组下标 = 文档ID，O(1)时间复杂度查询文档信息
             */
            std::vector<DocInfo> forward_index; 
            
            /**
             * @brief 倒排索引：哈希表映射关键词到倒排拉链
             * @details key = 关键词（小写），value = 该关键词的倒排拉链，O(1)时间复杂度查询倒排信息
             */
            std::unordered_map<std::string, InvertedList> inverted_index;

        private:
            // -------- 单例模式实现 --------
            //1.私有构造函数，禁止外部实例化
            Index(){} 
            //2.禁用拷贝构造函数，防止多实例
            Index(const Index&) = delete; 
            //3.禁用赋值运算符，防止多实例
            Index& operator=(const Index&) = delete;

            //4.单例实例指针（全局唯一）
            static Index* instance;
            
            //5.互斥锁，保证单例初始化的线程安全
            static std::mutex mtx;

        public:
            //1.析构函数，释放资源（此处无显式资源需要释放）
            ~Index(){}

        public:
            // -------- 单例获取接口 --------
            /**
             * @brief 获取索引类的单例实例（懒汉模式+双重检查锁）
             * @return Index* 全局唯一的索引实例指针
             * @details 双重检查锁：
             *          1. 外层检查：避免每次调用都加锁（提升效率）
             *          2. 内层检查：保证多线程下仅初始化一次
             *          3. 加锁：保证初始化过程的线程安全
             */
            static Index* GetInstance()
            {
                //1.外层检查（无锁，提升效率）
                if(instance == nullptr)
                {          
                    //1.1：加锁保证线程安全
                    mtx.lock(); 

                    //1.2：内层检查（防止多线程重复初始化）        
                    if(instance == nullptr){      
                        instance = new Index();
                    }

                    //1.3：解锁
                    mtx.unlock();                
                }
                //2.
                return instance;
            }

            // -------- 索引查询接口 --------
            /**
             * @brief 根据文档ID查询正排索引，获取文档完整信息
             * @param doc_id 待查询的文档ID
             * @return DocInfo* 文档信息指针（nullptr表示ID越界）
             */
            DocInfo *GetForwardIndex(uint64_t doc_id)
            {
                //1.检查ID是否越界
                if(doc_id >= forward_index.size())
                { 
                    std::cerr << "doc_id out range, error!" << std::endl;
                    return nullptr;
                }

                //2.返回对应文档的指针（O(1)）
                return &forward_index[doc_id];
            }

            /**
             * @brief 根据关键词查询倒排索引，获取倒排拉链
             * @param word 待查询的关键词（需小写）
             * @return InvertedList* 倒排拉链指针（nullptr表示关键词不存在）
             */
            InvertedList *GetInvertedList(const std::string &word)
            {
                //1.哈希表查找（O(1)）
                auto iter = inverted_index.find(word); 

                //2.关键词不存在
                if(iter == inverted_index.end())
                {     
                    std::cerr << word << " have no InvertedList" << std::endl;
                    return nullptr;
                }

                //3.返回倒排拉链的指针
                return &(iter->second);  
            }

            // -------- 索引构建接口 --------
            /**
             * @brief 构建正倒排索引的核心接口
             * @param input 预处理后的文档数据文件路径（如：data/raw_html/raw.txt）
             * @return bool 构建成功返回true，文件打开失败返回false
             * @details 读取预处理后的文档数据，逐行解析并构建：
             *          1. 先构建正排索引（DocInfo）
             *          2. 再基于正排索引构建倒排索引
             */
            bool BuildIndex(const std::string &input)
            {
                //1.以二进制模式打开输入文件（避免换行符转换）
                std::ifstream in(input, std::ios::in | std::ios::binary);
                if(!in.is_open()){ 
                    std::cerr << "sorry, " << input << " open error" << std::endl;
                    return false;
                }

                //2.存储每行文档数据（title\3content\3url）
                std::string line;    

                //3.已处理的文档数量
                int count = 0;      
                //4.逐行读取文档数据
                while(std::getline(in, line))
                {
                    //4.1：构建当前行的正排索引
                    DocInfo * doc = BuildForwardIndex(line);
                    if(doc == nullptr){ 
                        std::cerr << "build " << line << " error" << std::endl;
                        continue;
                    }

                    //4.2：基于正排索引构建倒排索引
                    BuildInvertedIndex(*doc);

                    //4.3：统计并打印进度日志
                    count++;
                    LOG(NORMAL, "当前的已经建立的索引文档: " + std::to_string(count));
                }
                //5.
                return true;
            }

        private:
            // -------- 内部辅助函数 --------
            /**
             * @brief 构建单条文档的正排索引
             * @param line 预处理后的单行文档数据（title\3content\3url）
             * @return DocInfo* 新构建的文档信息指针（nullptr表示解析失败）
             * @details 步骤：
             *          1. 切割行数据，提取title/content/url
             *          2. 填充DocInfo结构体，生成文档ID（数组下标）
             *          3. 将DocInfo插入正排索引数组，返回指针
             */
            DocInfo *BuildForwardIndex(const std::string &line)
            {
                //1.切割行数据：按分隔符\3拆分出title/content/url
                std::vector<std::string> results;

                //2.行内字段分隔符（与预处理阶段一致）
                const std::string sep = "\3";   

                //3.调用工具类切割字符串
                ns_util::StringUtil::Split(line, &results, sep); 
                if(results.size() != 3){ // 格式错误（必须包含3个字段）
                    return nullptr;
                }

                //4.填充DocInfo结构体
                DocInfo doc;
                doc.title = results[0];   // 标题字段
                doc.content = results[1]; // 内容字段
                doc.url = results[2];     // URL字段
                
                doc.doc_id = forward_index.size();  // 文档ID = 正排索引数组当前长度（插入后下标即为ID）

                //5.插入正排索引数组（std::move减少拷贝）
                forward_index.push_back(std::move(doc)); 

                //6.返回新插入的文档指针（back()返回最后一个元素）
                return &forward_index.back();
            }

            /**
             * @brief 基于单条文档构建倒排索引
             * @param doc 已构建完成的正排索引文档信息
             * @return bool 构建成功返回true
             * @details 核心步骤：
             *          1. 对标题/内容分别分词
             *          2. 统计词频（标题词权重更高）
             *          3. 计算每个关键词的权重，构建倒排元素
             *          4. 将倒排元素插入对应关键词的倒排拉链
             */
            bool BuildInvertedIndex(const DocInfo &doc)
            {
                //1.临时结构体：统计单个关键词在标题/内容中的出现次数
                struct word_cnt
                {
                    int title_cnt;    // 关键词在标题中的出现次数
                    int content_cnt;  // 关键词在内容中的出现次数

                    // 构造函数，初始化次数为0
                    word_cnt():title_cnt(0), content_cnt(0){}
                };
                
                //2.词频统计哈希表：key=关键词（小写），value=词频统计结构体
                std::unordered_map<std::string, word_cnt> word_map;

                // -------- 步骤1：标题分词与词频统计 --------
                //1.
                std::vector<std::string> title_words;
                //2.调用结巴分词工具，对标题进行分词
                ns_util::JiebaUtil::CutString(doc.title, &title_words);
                
                //3.统计标题中每个关键词的出现次数（统一转为小写，避免大小写敏感）
                for(std::string s : title_words)
                {
                    //3.1：转为小写（Boost字符串工具）
                    boost::to_lower(s); 

                    //3.2：词频+1（不存在则自动初始化）
                    word_map[s].title_cnt++; 
                }

                // -------- 步骤2：内容分词与词频统计 --------
                //1.
                std::vector<std::string> content_words;
                //2.调用结巴分词工具，对内容进行分词
                ns_util::JiebaUtil::CutString(doc.content, &content_words);
                
                //3.统计内容中每个关键词的出现次数（统一转为小写）
                for(std::string s : content_words)
                {
                    boost::to_lower(s);
                    word_map[s].content_cnt++;
                }

                // -------- 步骤3：计算权重并构建倒排索引 --------
                //1.权重计算系数：标题词权重远高于内容词
#define X 10 // 标题词权重系数
#define Y 1  // 内容词权重系数

                //2.遍历所有关键词的词频统计结果
                for(auto &word_pair : word_map)
                {
                    //2.1：构建倒排元素
                    InvertedElem item;
                    item.doc_id = doc.doc_id;           // 文档ID
                    item.word = word_pair.first;        // 关键词（小写）

                    //2.2：计算权重：标题词权重*X + 内容词权重*Y
                    item.weight = X*word_pair.second.title_cnt + Y*word_pair.second.content_cnt;

                    //2.3：将倒排元素插入对应关键词的倒排拉链（std::move减少拷贝）
                    InvertedList &inverted_list = inverted_index[word_pair.first];
                    inverted_list.push_back(std::move(item));
                }

                //3.释放宏定义，避免作用域污染
#undef X  
#undef Y

                //4.
                return true;
            }
    };

    // ===================== 静态成员初始化 =====================
    //1.单例实例指针初始化（全局唯一）
    Index* Index::instance = nullptr;
    
    //2. 互斥锁初始化
    std::mutex Index::mtx;
}