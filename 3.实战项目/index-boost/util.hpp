#pragma once 

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <boost/algorithm/string.hpp>  // Boost字符串处理库（切割、大小写转换等）

#include "log.hpp"                     // 自定义日志库（打印不同级别日志）
#include "cppjieba/Jieba.hpp"          // 结巴分词库头文件（中文分词核心）

// ===================== 工具类命名空间 =====================
/**
 * @brief 工具类命名空间，封装搜索引擎预处理阶段的核心工具：
 *        1. FileUtil：文件读取工具
 *        2. StringUtil：字符串切割工具
 *        3. JiebaUtil：中文分词工具（含停用词过滤，单例模式）
 */
namespace ns_util
{
    // ===================== 文件操作工具类 =====================
    /**
     * @brief 文件读取工具类（静态类，无需实例化）
     * @details 提供通用的文件读取接口，将文件内容读取到字符串中
     */
    class FileUtil
    {
        public:
            /**
             * @brief 读取文件内容到字符串（文本模式）
             * @param file_path 输入参数：待读取的文件路径
             * @param out 输出参数：存储文件内容的字符串指针
             * @return bool 读取成功返回true，文件打开失败返回false
             * @note 1. 按行读取文件，自动拼接所有行内容
             *       2. getline会自动跳过换行符，最终out中无换行符（适合HTML内容读取）
             */
            static bool ReadFile(const std::string &file_path, std::string *out)
            {
                //1.以文本模式打开文件（ios::in：只读）
                std::ifstream in(file_path, std::ios::in);
                if(!in.is_open())
                { 
                    std::cerr << "open file " << file_path << " error" << std::endl;
                    return false;
                }

                //2.存储每行读取的内容
                std::string line; 
                //3.逐行读取文件
                while(std::getline(in, line)) //getline返回流对象，while会隐式转换为bool（流有效则true）
                { 
                    *out += line; // 将每行内容拼接到输出字符串
                }

                //4.关闭文件，释放资源
                in.close(); 
                return true;
            }
    };

    // ===================== 字符串处理工具类 =====================
    /**
     * @brief 字符串切割工具类（静态类，无需实例化）
     * @details 基于Boost库实现通用的字符串切割功能，适配搜索引擎的字段拆分需求
     */
    class StringUtil
    {
        public:
            /**
             * @brief 按指定分隔符切割字符串
             * @param target 输入参数：待切割的目标字符串
             * @param out 输出参数：存储切割结果的vector指针
             * @param sep 输入参数：分隔符（支持多字符分隔，如"\3,;"）
             * @note 使用Boost的split函数，开启token_compress_on（压缩连续分隔符）
             */
            static void Split(const std::string &target, std::vector<std::string> *out, const std::string &sep)
            {
                // Boost字符串切割核心接口：
                // boost::split(输出容器, 输入字符串, 分隔符判断器, 切割选项)
                // boost::is_any_of(sep)：支持sep中的任意字符作为分隔符
                // boost::token_compress_on：压缩连续分隔符（避免空字符串元素）
                boost::split(*out, target, boost::is_any_of(sep), boost::token_compress_on);
            }
    };

    // ===================== 中文分词工具类（结巴分词） =====================
    /**
     * @brief 结巴分词字典路径常量（UTF-8编码）
     * @details 字典文件说明：
     *          - jieba.dict.utf8：主词典（基础词库）
     *          - hmm_model.utf8：HMM模型（处理未登录词）
     *          - user.dict.utf8：用户自定义词典（可扩展领域词汇）
     *          - idf.utf8：IDF权重文件（关键词提取用）
     *          - stop_words.utf8：停用词词典（过滤无意义词汇，如"的"、"了"）
     */
    const char* const DICT_PATH = "./dict/jieba.dict.utf8";
    const char* const HMM_PATH = "./dict/hmm_model.utf8";
    const char* const USER_DICT_PATH = "./dict/user.dict.utf8";
    const char* const IDF_PATH = "./dict/idf.utf8";
    const char* const STOP_WORD_PATH = "./dict/stop_words.utf8";

    /**
     * @brief 中文分词工具类（单例模式）
     * @details 基于结巴分词库实现：
     *          1. 单例模式：保证分词器全局唯一（避免重复加载字典，节省内存）
     *          2. 停用词过滤：自动过滤无意义词汇，提升索引质量
     *          3. 搜索专用分词：CutForSearch（更细粒度的分词，适合搜索引擎）
     */
    class JiebaUtil
    {
        private:
            // -------- 核心成员变量 --------
            //1.结巴分词器实例（核心分词对象）
            cppjieba::Jieba jieba;

            //2.停用词哈希表（快速查询是否为停用词） 
            std::unordered_map<std::string, bool> stop_words; //key=停用词（UTF-8），value=占位符（true），O(1)查询

        private:
            // -------- 单例模式实现 --------
            //1.私有构造函数，初始化结巴分词器（加载结巴分词的5个字典文件，完成分词器初始化）
            JiebaUtil():jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH){}
            
            //2.禁用拷贝构造函数，防止多实例
            JiebaUtil(const JiebaUtil&) = delete;

            //3.单例实例指针（全局唯一）
            static JiebaUtil *instance;

        public:
            /**
             * @brief 获取分词工具类的单例实例（懒汉模式+线程安全）
             * @return JiebaUtil* 全局唯一的分词器实例指针
             * @details 1. 静态局部互斥锁：保证线程安全（C++11后静态局部变量初始化线程安全）
             *          2. 双重检查锁：避免重复初始化
             *          3. InitJiebaUtil：初始化停用词表（仅第一次调用）
             */
            static JiebaUtil* get_instance()
            {
                //1.静态局部互斥锁（仅初始化一次）
                static std::mutex mtx; 
                //2.外层检查（无锁，提升效率）
                if(nullptr == instance)
                {       
                    //2.1：加锁保证线程安全
                    mtx.lock();                 
                    //2.2：内层检查（防止多线程重复初始化）
                    if(nullptr == instance)
                    {  
                        instance = new JiebaUtil();
                        instance->InitJiebaUtil(); // 初始化停用词表
                    }
                    //2.3：解锁
                    mtx.unlock();               
                }

                //3.直接返回单例实例指针
                return instance;
            }

            /**
             * @brief 初始化停用词表（私有接口，仅单例初始化时调用）
             * @details 读取停用词字典文件，加载到stop_words哈希表中
             */
            void InitJiebaUtil()
            {
                //1.打开停用词文件（文本模式）
                std::ifstream in(STOP_WORD_PATH);
                if(!in.is_open())
                {
                    LOG(FATAL, "load stop words file error");
                    return;
                }

                //2.存储每行的停用词
                std::string line; 
                //3.逐行读取停用词（每行一个停用词，UTF-8编码）
                while(std::getline(in, line))
                {
                    stop_words.insert({line, true}); // 插入哈希表
                }

                //4.关闭文件
                in.close(); 
            }

            /**
             * @brief 分词核心辅助函数（内部接口）
             * @param src 输入参数：待分词的字符串（UTF-8编码）
             * @param out 输出参数：存储分词结果的vector指针（已过滤停用词）
             * @details 1. CutForSearch：搜索专用分词（细粒度，适合搜索引擎）
             *          2. 停用词过滤：遍历分词结果，删除停用词
             */
            void CutStringHelper(const std::string &src, std::vector<std::string> *out)
            {
                //1.结巴分词：搜索专用分词（更细粒度，如"程序员"→"程序|程序员|员"）
                jieba.CutForSearch(src, *out);
                
                //2.遍历分词结果，过滤停用词
                for(auto iter = out->begin(); iter != out->end(); )
                {
                    //2.1：查询当前词是否为停用词
                    auto it = stop_words.find(*iter);

                    //2.2：是停用词，删除该元素，迭代器指向删除后的下一个元素
                    if(it != stop_words.end()){
                        iter = out->erase(iter);
                    }

                    //2.3：非停用词，迭代器后移
                    else{
                        iter++;
                    }
                }
            }

        public:
            /**
             * @brief 对外暴露的分词接口（静态方法，无需实例化）
             * @param src 输入参数：待分词的字符串（UTF-8编码）
             * @param out 输出参数：存储分词结果的vector指针
             * @details 封装单例调用，简化外部使用：
             *          1. 获取单例实例
             *          2. 调用内部分词辅助函数
             */
            static void CutString(const std::string &src, std::vector<std::string> *out)
            {
                ns_util::JiebaUtil::get_instance()->CutStringHelper(src, out);
            }
    };

    // ===================== 静态成员初始化 =====================

    //分词器单例实例指针初始化（全局唯一）
    JiebaUtil *JiebaUtil::instance = nullptr;
}

