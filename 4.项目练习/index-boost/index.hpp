#ifndef INDEX_HPP
#define INDEX_HPP

#include "util.hpp"

//首先定义命名空间
namespace ns_index
{
    //在命名空间中先定义一些结构体
    /*
         1. 正排索引结构体
         2. 倒排索引元素结构体
    */
    struct DocInfo
    {
        //文档的信息
        std::string title;    //文档的标题
        std::string content;  //文档的内容
        std::string url;      //文档的url

        //文档的ID
        uint64_t doc_id;
    }


    struct InvertedElen
    {
        //1.关键字所在文档的ID
        uint64_t doc_id;

        //2.关键字本身
        std::string word;

        //3.关键字在该文档中的权重
        int weight;


        //构造函数初始化权重为0
        InvertedELem():weight(0){}
    }



    //1.接下来我们来创建索引类：“索引类：class Index
    class Index
    {
        private:
        //正排索引：vector数组中存储所有文档信息
        std::vector<DocInfo_t> forward_index;

        //倒排索引：哈希表映射：关键词 ---> 倒排拉链
        std::unordered_map<std::string,vector<InvertedElen>> inverted_index;


        private:
        //单例模式实现
        //1.私有构造函数
        index(){}

        //2.禁用构造函数
        index(const index&)=delete;

        //3.单例实例指针
        static index *instance;
        /*
        一句话结论：保存单例唯一实例的指针，必须是 static 成员变量！
            private:
                static Singleton* _inst; // ✅ 必须 static
            为什么必须 static？3 个核心原因

            1. static 是属于 “类” 的，不是属于 “对象” 的
            普通成员变量：属于每一个对象，每个对象一份
            static 成员变量：属于整个类，全类共享一份
            单例要的就是：全类只有一个实例！ → 只有 static 能做到！

            2. GetInstance () 是静态函数，只能访问静态变量
            你获取单例的接口是 static：
            static Singleton* GetInstance()
            {
                return _inst; // 这里只能访问 static 变量！
            }
            静态函数没有 this 指针，不能访问任何普通成员变量
            只能访问 静态成员变量
            所以 _inst 必须是 static！

            3. 全局唯一，生命周期贯穿整个程序
            static 成员变量：程序启动就创建、程序结束才销毁、全程只有一个
            正好符合单例：全局唯一、一直存在、到处使用
        */


        //4.禁用赋值运算符
        Index& operator(const Index&)=delete;
        //5.加上互斥锁保证单例
        static std::mutex mtx;
        /*
        一、为什么要禁用 赋值运算符 operator=？
            Index& operator=(const Index&) = delete;
            核心原因：防止有人把单例对象 “赋值” 给别人，造出第二个实例！
            举个危险例子（如果不禁用）：
            Index& ins1 = Index::GetInstance();
            Index& ins2 = Index::GetInstance();

            ins1 = ins2; // 赋值操作！
            虽然大家都是单例，但赋值运算符是 C++ 默认生成的！
            默认的赋值会逐字节拷贝对象内部内容！

            后果：破坏单例唯一性、对象内部数据错乱、程序直接崩溃
            所以：单例 = 只能有一个，不能拷贝，不能赋值！ 必须把拷贝构造 + 赋值运算符全部禁用！


        二、为什么要加 互斥锁 mutex？
            static std::mutex mtx;
            核心原因：保证多线程环境下，单例只创建一次，不会创建出多个对象！也就是 线程安全！
            多线程危险场景（不加锁会出大事）：线程 A 和 线程 B 同时调用：
            if (instance == nullptr) {
                instance = new Index;
            }

            可能出现：
                A 判断 instance == nullptr
                B 也判断 instance == nullptr
            A 和 B 都去 new
            最终创建了 2 个对象！单例彻底失效！
            加锁后：std::lock_guard<std::mutex> lock(mtx);
            只允许一个线程进入创建代码
            保证永远只创建一个对象！
        */

        public:

        //1.实现：“获取索引类index的单例实例”
        static Index* GetInstace()
        {
            //1.外层if提速
            if(instance ==nullptr)
            {
                //1.1：加锁
                mtx.lock();

                //1.2：内层if安全
                if(instance==nullptr)
                {
                    //第一步：实例化单例对象
                    instance =new index();
                }

                //1.3：解锁
                mtx.unlock()
            }

            //2.返回实例
            return instamce;
        }


        //2.实现：“根据文件ID查询正排索引，获取文档的完整信息”：
        /*
            关于函数的说明信息：
            完整的文档的信息 ---> “正排索引结构体” ---> 这个结构体保存在正排索引的vector容器中
            从本质上来讲：文件的ID 可以理解为 vector容器的下标
        */
        DocInfo* GetForwardIndex(uint64_t doc_id)
        {
            //1.首先检查文档ID是否越界
            if(doc_id>=forward_index.size())
            {
                std::cerr<<"文档ID越界"<<std::endl;
                return nullptr;
            }

            //2.然后就是返回对应的正排索引结构体指针
            return &forward_index[doc_id];
        }


        //3.实现：“根据关键词查询倒排索引，获取倒排拉链”
        std::vector<InvertedElem>* GetInvertList(const std::string &word)
        {
            //1.首先直接使用哈希表进行查找
            auto it = inverted_index.find(word);
            //inverted_index[word]; 这里还是要注意不能使用[]的形式直接进行查找
            // if(it==nullptr)
            /*
                find() 返回的是 迭代器（iterator）
                不是指针，不是地址，不能用 nullptr 判断
            */
            if(it == invected_index.end())
            {
                std::cerr<<"根据该关键词"<<word<<"没有找到对应的倒排拉链"<<std::endl;
                return nullptr;
            }

            //2.最后直接返回找到结果就行了
            return &(it->second);
            
        }


        //4.实现：“构建正倒排的核心接口”
        bool BuildIndex(const std::string& input)
        //参数说明：input指的是经过解析预处理之后的文档的存放路径
        {
            //1.以二进制的形式打开文件
            std::ifstream in(input,std::ios::in|std::ios::binary)
            if(!in.is_open())
            {
                std::cerr<<"文档的打开失败"<<std::endl;
                return false;
            }

            //2.定义变量记录已经处理的文档数量
            int count =0;

            //3.逐行读取文档数据
            std::string line;
            while(getline(in,line)==true)
            {
                //3.1：基于单行文档数据构建正排索引
                DocInfo *doc = BuildForwardIndex(line);
                if(doc == nullptr)
                {
                    std::cerr<<"正排索引构建失败"<<std::endl;
                    return false;
                }

                //3.2：基于正排索引构建倒排索引
                bool ret = BuildInvertIndex(*doc);
                if(ret==false)
                {
                    std::cerr<<"倒排索引构建失败"<<std::endl;
                    return false;
                }


                //3.3：已处理的文档数量+1并打印日志
                count++;
                LOG(NORMAL, "当前的已经建立的索引文档: " + std::to_string(count));

            }
            //4.最后返回true
            return true;
            
        }




        private:
        //内部的辅助函数：
        //1.实现：“构建单条文档正排索引”
        /*
            首先要对这个参数和返回值有所了解：
                1）参数：解析预处理完之后的单行的数据内容
                2）返回值；新构建的文档信息指针  
        */
        DocInfo* BuildForwardIndex(const std::string line)
        {
            //1.调用工具类的方法切割字符串
            std::vector<std::string> results;
            const std::string sep="\3";
            ns_util::StringUtil::Split(line,sep,&results);
            if(results.size()!=3)
            {
                return nullptr;
            }


            //2.填充DocInfo结构体
            DocInfo doc;
            doc.title = results[0];
            doc.body = results[1];
            doc.url = results[2];

            //3.正排索引的当前的长度 == 该文档的ID  == 文档插入后的数组下标
            doc.doc_id = forward_index.size();

            //4.插入正排索引数组
            forward_index.push_back(std::move(doc));

            //5.返回新插入的文档指针
            // return &doc; //返回了局部变量的地址（最致命！）
            return &forward_index.back();
            /*
                forward_index 是类成员变量（全局 / 堆上）
                back() 拿到数组里最后一个元素
                返回它的指针 = 安全有效，不会失效！

            */
        }


        //2.实现：“根据单条文档信息创建倒排索引”
        bool BuildInvertIndex(const DocInfo &doc)
        {
            //1.定义词频统计结构体，用来统计关键词在标题和内容中出现的次数
            struct word_cnt
            {
                int title_cnt;
                int content_cnt;


                //构造函数将其两者都初始化为0
                word_cnt():title_cnt(0),content_cnt(0){}
            }


            //2.定义词频统计哈希表：关键词 ---> 词频统计结构体
            std::unoredered_map<std::string,struct word_cnt> word_map

            //3.统计标题中关键词出现的次数
            //3.1：调用jieba分词工具对标题进行分词
            std::vector<std::string> title_word;
            ns_util::JiebaUtil::CutString(doc.title,&title_word);
            //3.2：遍历每一个分词统计关键词出现的次数
            for(std::string s:title_word)
            {
                //第一步：将分词转成的小写
                boost::to_lower(s);

                //第二步：关键词在标题中出现的次数+1
                word_map[s].title_cnt++;
            }
            

            //4.统计内容中关键词出现的次数
            //4.1：调用jieba分词工具对内容进行分词
            std::vector<std::string> content_word;
            ns_util::JiebaUtil::CutString(doc.content,&content_word);
            //4.2：遍历每一个分词统计关键词出现的次数
            for(std::string s:content_word)
            {
                //第一步：将分词转化成小写
                boost::to_lower(s);


                //第二步：关键词在内容中出现的次数+1
                word_map[s].content_cnt++;
            }

            //5.计算权重并构建倒排索引
            //5.1：定义权重计算系数
            #define X 10
            #define Y 1

            //5.2：遍历所有关键词的词频统计结果
            for(auto &word_pair:word_map)
            {
                //第一步：构建倒排元素并填充关键字所在文档的ID和关键字本身  
                /*
                    倒排元素结构体中有三个成员：
                        1. 关键字所在文档的ID  ：doc_id
                        2. 关键字本身          ：word
                        3. 关键字的权重        :  weight
                */

                InvertedElen elem;
                elem.doc_id = doc.doc_id;
                elem.word = word_pair.first;

                //第二步：计算权重
                elem.weight=x*word_pair.second.title_cnt + Y*word_pair.second.content_cnt;

                //第三步：将倒排元素添加到对应关键词的倒排拉链
                // inverted_index.insert(std::make_pair(word_pair.first,elem));
                inverted_index[word_pair.first].push_back(std::move(elem));

                /*
                写法 1（你注释掉的）❌ 大错特错
                    inverted_index.insert(
                        make_pair(word_pair.first, elem)
                    );
                    为什么错？
                    insert 是直接放入一个键值对
                    你把 elem（单个元素）直接塞给 vector 类型不匹配！
                    要求：vector<InvertedElem>
                    你给：InvertedElem
                    结果： 编译失败！ 这是类型错误，绝对不能用！
                */

            }


            //6.释放宏定义并返回true
            #undef x
            #undef Y
            return true;
        }



    };

    //由于这个类使用了单例模式：也就是说这个类中一定会有静态的成员属性，所以它们只能在类外进行初始化了
    // 1）静态单例实例指针   2）静态互斥锁
    //他们的初始化很多人都搞不定，口诀就是：1.去掉static   2.变量名称前面添加 “类名::”
    // static index* instance;
    // static std::mutex mtx;
    Index* Index::instance = nullptr;
    std::mutex Index::mtx;

}


#endif