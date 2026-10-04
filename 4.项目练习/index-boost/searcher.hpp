#ifndef SEARCHER_HPP
#define SEARCHER_HPP

#include "util.hpp"

namespace ns_searcher
{
    //首先定义一个结构体：“倒排元素扩展结构体”
    struct InvertedElemPrint
    {
        //1.文档ID
        uint64_t doc_id;


        //2.文档的总相关的权重
        int weight;
        //3.该文档匹配的关键词列表
        std::vector<std::string> word;

        /*
            1. 文档ID                 ---> 获取“文档的详细的信息”
            2. 文档的总相关权重        ---> 根据其进行降序排序
            3. 该文档匹配的关键词列表   ---> 获取摘要
        */


        //构造函数，初始化权重和文档ID
        InvertedElemPrint():doc_id(0),weight(0){}

    }


    //接下来实现：“搜索类：class Searcher”
    class Searcher
    {
        private:
        /*
            Searcher 是 “搜索服务员”，Index 是 “搜索的数据库 / 字典”。
                服务员要干活，必须手里拿着数据库！
        
        我用超级通俗的比喻讲，你现在做的是搜索引擎：
            1. Index（索引类）= 一本超级词典
            里面存了：所有文档（正排索引） 所有关键词对应的文档（倒排索引）
            没有它，搜索根本查不到任何东西！

            2. Searcher（搜索类）= 帮你查词典的服务员
            服务员要帮你搜索，必须先拿到这本词典！
            ns_index::Index* index;
            意思就是：给搜索服务员，准备一本词典的钥匙！



        为什么是指针？ Index* index;
        原因超级简单：
            Index 是单例！全局只有一个！
            我们不拷贝、不创建新的
            只需要用指针 “指向” 那个唯一的索引
            节省内存、速度快
        
        完整逻辑（你一看就通）
            程序启动
            ↓
            创建唯一的索引 Index（加载文档、建立正排/倒排）
            ↓
            Searcher 拿到 Index 的指针
            ↓
            用户搜索
            ↓
            Searcher 通过 index 指针查索引
            ↓
            返回结果
        */

        //1.索引实例指针
        ns_index::Index* index;


        public:
        //这里是不需要“构造函数 + 析构函数”
        /*
            索引实例由单例管理，无需手动释放
        */

        //1.实现：“搜索器初始化”
        void InitSearcher(const std::string input)
        /*
            参数说明：传入的是经过与处理过的文件的存放位置的路径
            函数任务：
               1. 创建单例实例
               2. 构建正倒排索引
        */
        {
            //1.创建单例实例
            index=ns_index::Index::Getinstance();
            LOG(NORMAL, "获取index单例成功...");

            //2.构建正倒排索引
            index->BuildIndex(input);
            LOG(NORMAL, "建立正排和倒排索引成功...");
        }


        //2.实现：“处理用户的搜索请求并生成JSON格式搜索结果”
        void search(const std::string &query,std::string &json_string)
        {
            //1.对搜索关键词进行分词
            std::vector<std::string> words;
            ns_util::JiebaUtil::CutString(query,&words);
 
            //2.创建哈希表：文档ID ---> 文档的权重+匹配关键词
            std::unordered_map<uint64_t,InvertedElemPrint> tokens_map;
            // unordered_map<uint64_t,struct InvertedElemPrint>
            /*
                为什么都对？（超级简单）
                这是 C 语言遗留下来的语法习惯：
                在 C 语言里
                    定义结构体后，使用时必须写 struct
                    struct InvertedElemPrint elem;

                在 C++ 里
                编译器做了优化，可以省略 struct
                InvertedElemPrint elem;
                所以：
                    写 struct → 兼容 C 语法，老式写法
                    不写 struct → C++ 推荐写法，更简洁
                    你们项目里统一用不带 struct的就行！
                unordered_map<uint64_t, InvertedElemPrint>
            */
            

            //3.遍历每个分词之后的关键词
            for(std::string word:words)
            {
                //3.1：将分词转换为小写
                boost::to_lower(word);

                //3.2：获取关键词的倒排拉链
                ns_index::std::vector<InvertedElem> *inverted_list= index->GetInvertedList(word);
                if(inverted_list == nullptr)
                {
                    continue;
                }

                //3.3：遍历该关键词的倒排拉链，并将文件的信息合并到tokens_map
                for(const auto it:*inverted_list)
                {
                    //第一步：通过该关键词对应文章ID获取InvertedElemPrint结构体
                    auto& item tokens_map[it.doc_id];

                    //第二步：填充InvertedElemPrint的成员
                    item.doc_id = it.doc_id;
                    item.words.push_back(it.word); //记录匹配的关键词
                    item.weight += it.weight;      //求累加权重
                }
                // 用户输入的关键词 ---> 分词 ---> 拉链 ---> 文档 ---> 文档ID ---> InvertedElemPrint结构体 --->填充
            }

            //4.将哈希表转化为vector方便进行排序
            std::vector<InvertedElemPrint> inverted_list_all;
            for(const auto &it:tokens_map)
            {
                inverted_list_all.push_back(std::move(it.second));
            }

            
            //5.按照相关性权重降序排序
            std::sort(inverted_list_all.begin(),inverted_list_all.end(),
                    [](const InvertedElemPrint &e1,const InvertedElemPrint &e2)
                    {
                        return e1.weight>e2.weight;
                    }
            );

            //6.遍历排序后的结果逐个构建Json::Value对象
            Json::Value root;
            for(const auto &item : inverted_list_all)
            {
                //6.1：根据文档ID通过正排索引获取文档的详细信息
                ns_index::DocInfo* doc=  index->GetForwardIndex(item.doc_id)
                if(doc == nullptr)
                {   
                    continue;
                }
                /*
                    这里提前使用文档ID通过正排索引获取文档的信息是为了：
                    获取这三样信息来构建Json::Value对象
                        1. 文档的标题
                        2. 文档的内容
                        3. 文档的url
                */

                //6.2：构建单个搜索结果的Json::Value对象
                Json::Value temp;
                temp["title"]=doc->title;
                temp["desc"]=doc->GetDesc(doc->content,item.words[0]); //文档摘要（取第一个匹配关键词的上下文）
                temp["url"]=doc->url;

                temp["id"]=(int)item->doc_id;
                temp["weight"]=item->weight;

                /*
                最核心、最直白的答案
                因为 doc_id 的类型是 uint64_t /size_t，不是 int！
                而 JSON 前端 / JS 只认 int，不认 uint64_t！

                1. 先看类型差异（关键！）
                你的 item.doc_id 类型是：
                    uint64_t doc_id;  // 无符号64位整数
                    // 或者 size_t doc_id;
                但 Json::Value 对数字的处理：
                    它能识别 int
                    它不认识 uint64_t /size_t（会当成奇怪的无符号数）
                如果你不加强转：
                    elem["id"] = item.doc_id;
                    可能出现：JSON 数字格式异常，前端 JS 解析出错，数字变成乱码 / 超大数

                2. 为什么 doc_id 是 uint64_t？
                因为：
                    doc_id = 正排索引数组的下标 = size_t
                    doc.doc_id = forward_index.size();
                    size() 返回的是 size_t / uint64_t 不是 int！

                3. 为什么要强转成 int？
                    ① JSON / JavaScript 不支持 uint64_t
                    JS 最大安全整数是 2^53-1 你传 uint64_t → 前端精度丢失！

                    ② 文档 ID 不可能超过 int 范围
                    你的文档数量：
                    最多几千、几万，绝对不会超过 2 亿（int 上限）
                    所以强转 100% 安全！
                */

                //6.3：将单个结果添加到根数组
                root.append(temp);
            }

            //7.序列化Json::Value对象为JSON串
            Json::FastWriter writer;
            *json_string = writer.write(root);
        }

        //2.实现：“生成文档摘要”
        std::string GetEsc(const std::string &content,const std::string &word)
        /*参数：
            1. 文档的纯文本的内容
            2. 匹配的关键字用户定位上下文
          返回值：生成摘要内容
        */
        {
            //1.摘要的截取规则（关键字前50，后100字符）
            const int prev_step = 50;
            const int next_step = 100;

            //2.查找关键字在内容中首次出现的位置
            /*
                auto it = content.find(word);
                if(it==std::string::npos) //返回的不是迭代器，是下标（size_t 类型数字）！找到 → 返回位置下标（如 10、20）
                {
                    std::cerr<<"在文档中未找到匹配的关键字"<<std::endl;
                    return std::string();
                }
            */
            /*
                最关键：string::find () 不支持自定义规则
                string::find()只能：完全相等匹配，不能忽略大小写！

                std::search()可以：自定义比较规则（忽略大小写、模糊匹配等）
                所以：做搜索、做忽略大小写匹配 → 必须用 std::search
            */
            auto it = std::search(contect.begin(),content.end()
            ,word.begin(),word.end()
        [](int x,int y){std::tolower(x)==std::tolower(y);})
            //查询左区间  查询右区间   关键字左区间 关键词右区间  自定义匹配规则
            if(it==content.end())
            {
                return "None1"
            }

            //3.计算关键词首次出现的位置距离内容开头的偏移量
            int pos = std::distance(content.begin(),it);

            //4.确定摘要的起始和结束位置
            int start=0;
            int end = content.size()-1;

            //5.修正摘要的起始和结束位置
            //5.1：如果关键词前有足够字节，起始位置后移prev_step
            if(pos > start+prev_next) start=pos-prev_next;
            //5.2：如果关键词前有足够字节，结束位置前移next_step
            if(pos < end-next_step) end=pos+next_step;
            //5.3：边界校验
            if(start>=end) return "None2";

            //6.截取[start,end)范围的子串作为摘要
            std::string desc = conntent.surstr(start,end-start);
            desc+="......";
            return desc;
        }
    };
}



#endif 