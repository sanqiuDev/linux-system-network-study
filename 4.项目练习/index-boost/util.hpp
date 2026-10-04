#ifndef UTIL_HPP
#define UITL_HPP

#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include "cppjieba/Jieba.hpp"
/*
   之前写五子棋的项目的时候，我记得我们下载过websocket的什么的，
   但是也没有什么将一个文件夹放在项目什么的，但是这个cppjieba为什么还要将cppjieba放在项目中

   先说结论：为什么 websocket++ 不用放项目里，cppjieba 却要放？
   核心原因就一个：它们的安装 / 使用方式不一样。
    库	            安装/使用方式	           为什么不用放项目里？
    websocket++	    纯头文件库 + 系统级依赖	   它的头文件安装在了系统路径里，编译器默认能找到。
    cppjieba	    未安装的第三方源码库	   它没有被安装到系统里，所以你必须把它的源码/头文件放在项目里，让编译器能找到。

    2. 先回顾一下你之前的 websocket++ 是怎么工作的？
    在你的五子棋项目中，你写的代码可能是这样的：#include <websocketpp/server.hpp>
    编译器之所以能找到它，是因为：你在系统里（比如通过 apt-get 或源码编译）安装了 websocket++
    安装程序会把头文件复制到系统的标准路径（比如 /usr/include/）。
    当你用 <> 包含头文件时，编译器会自动去这些标准路径里找，所以你不用管它在哪。
    简单说：它已经 “住进” 了系统，你只要用尖括号 <> 喊它名字，它就会答应。

    3. 再看现在的 cppjieba 为什么要放在项目里？
       你现在的项目结构是：
        index-boost/
        ├── cppjieba/   ← 你把库源码放在了这里
        │   └── Jieba.hpp
        └── util.hpp
        然后你在 util.hpp 里这样写：#include "cppjieba/Jieba.hpp"
        这行代码用的是双引号 " "，它的含义是：“去当前文件所在的目录里，找 cppjieba/ 这个子文件夹，然后找 Jieba.hpp”。
        你把 cppjieba 文件夹放进项目，本质上就是告诉编译器：“它就在这，你不用去系统里找了。”
*/

//细节这里使用了一个命名空间，之前我们在五子棋的项目中没有进行使用，但是对于工具文件还是建议使用命名空间的
namespace ns_util
{
    //实现第一个类就是：“文件操作类：file_util”
    class FileUtil
    {
        public:
        static bool ReadFile(const std::string &filepath,std::string *buffer) //注意细节对这个输出参数使用指针进行定义
        /* 这里我们对函数参数的介绍是：
              1. 如果函数参数是在函数中是不能修改的 ---> 添加const
              2. 如果函数参数是需要带回来输出内容的 ---> 添加指针
              3. 其余情况有事没事都可以添加引用
        */
        /*
            回想我们在写五子棋那个项目的时候：也写过“文件工具函数”，——将指定文件路径下的文件中的内容全部读到缓冲区中
                1.文件路径
                2. 缓冲区（本质上就是是一个字符串）
        */
       {
            //1.首先要以二进制的形式打开指定路径的文件
            std::ifstream in(filepath,std::ios::in); //这里需要添加头文件<fstream>
            /*
                首先：看到ifstream 判定是“读取文件中的内容”
                其次：看打开方式是std::ios::in 判定是使用常规“文本方式读取”

                默认打开方式 = ios::in（只读） + ios::binary（二进制模式）
                ifstream in(ios::binary); == ifstream in(ios::in | ios::binary);
            */
            if(in.is_open()==false)
            {
                std::cerr<<"打开文件"<<filepath<<"失败"<<std::endl;   //这里需要引入头文件<iostream>
                return false;
            }
            /*
                先直接回答你的 3 个核心问题
                    1. ifstream 确实是一个类，in 是一个文件输入流对象
                    2. ios 是一个类
                    3. ios::in 是一个「文件打开模式常量」（不是对象，不是函数，就是一个标志位）
                下面我把整个体系给你讲得明明白白。
            
                C++ 文件流最核心、最常用的就 4 个类，功能完全平行：
                类名	          作用	                   对应操作
                ifstream	      读文件	               input file stream
                ofstream	      写文件	               output file stream
                fstream	          可读可写	               file stream
                stringstream	   读写字符串（不是文件）	内存流

                         C++ 流继承链如下：
                            ios_base
                               ↓
                       ios (输入输出流基类)
                 ├─────────────┬─────────────┐
               istream      ostream       iostream
                  ↓            ↓             ↓
               ifstream     ofstream       fstream

                 ios = 所有流的祖宗
                 istream = 专门负责读
                 ostream = 专门负责写
                 ifstream / ofstream / fstream = 包装成文件操作



            我逐词拆解给你看：
                ① std::ifstream：是一个类作用：创建一个用来读文件的对象。
                ② in：是你定义的对象名

                ③ (file_path, std::ios::in)：调用 ifstream 的构造函数
                    第 1 参数：文件路径
                    第 2 参数：文件打开方式
            
            4. 重点：ios 是什么？ios::in 又是什么？
                ✅ ios 是一个类：它是所有输入输出流的基类。
                ✅ ios::in 不是函数、不是对象：它是 ios 类里定义的一个常量（打开模式标志）
            */

         
            //2.逐行读取文件中的内容 --->>核心是：1）while循环   2）getline函数：getline(文件输入流对象，字符缓冲区)
            std::string line;
            while(getline(in,line)==true)  //getline返回流对象，while会隐式转换为bool（流有效则true）
            {
                *buffer+=line;
            }

            //3.关闭文件是释放资源
            in.close();
            return true;
       }
       

    };


    //接下我们来实现下一个类的定义：“字符串处理的工具类：class StringUtil”
    class StringUtil
    {
        public:
        static void Split(const std::string &src,const std::string &sep, std::vector<std::string> *res) //指定头文件<vector>
        /*
            这里我们之前在五子棋的项目中写过“字符串的分割操作”，当时对于分割函数的参数是
               1. 待分割的源字符串
               2. 分割符字符串
               3. 存储分割后的子字符串集合
        */
       {
            boost::split(*res,src,boost::is_any_of(sep),boost::token_compress_on); //需要添加boost的头文件
            /*
                boost::split(输出容器, 输入字符串, 分隔符判断器, 切割选项)
                    
                4 个参数，我一个一个讲
                    第 1 个参数：*out：输出容器（就是你说的带出数据的参数 = 出参）
                    类型：vector<string>&
                    作用：切割后的所有小字符串都会放进这里面，你这里用的是指针 *out，所以要解引用
                    
                    第 2 个参数：target：输入字符串（要被切割的原始字符串）
                    就是你想切的那一大段文本

                    第 3 个参数：boost::is_any_of(sep)
                    这是一个 “分隔符匹配器”作用：只要字符出现在 sep 里，就当作分隔符。
                    sep = ",;|"：那么 逗号、分号、竖线 都会被当成分隔符
                    这就是它强大的地方：支持一次性用多个符号切割。

                    第 4 个参数：boost::token_compress_on
                    压缩连续分隔符："a,,b;;;c"
                    如果不开压缩 → 会切出空字符串：["a", "", "b", "", "", "c"]
                    开了压缩 → 自动合并连续分隔符：["a", "b", "c"]


                boost::is_any_of(sep)→ 不是函数！不是类！→ 是一个 【模板函数】（你可以理解成：普通函数）
                boost::token_compress_on→ 不是函数！不是类！→ 是一个 【枚举常量】（你可以理解成：标志位 / 宏定义）

                最直观对比（一看就懂）
                名称	                类型        写法	        属于
                is_any_of	            函数	   必须写 ()        函数调用
                token_compress_on	    常量	   不能写 ()	    标志位
            */
       }
    };


    //首先我们需要先定义一些：“结巴分词的字典路径常量”
    /*
        为什么要写这些字典路径？每个文件是干嘛的？为什么必须放路径？
        你现在看到的代码，是 cppjieba 中文分词库 的标准写法，所有公司用结巴分词都必须这么写，不是随便定义的。

        一、最核心一句话回答你
        结巴分词不是凭空分词的，它必须靠「词典文件」才能工作。
        你写这些路径，就是告诉程序：「字典在哪里，我要加载它们才能分词」就像：
            你要查字典，必须告诉程序字典放哪个文件夹
            没有字典 → 分词直接崩溃
            路径错了 → 加载失败，分不出词


        二、每个路径到底是干嘛的？（超级通俗版）
        我一个一个给你讲人话：
            1. DICT_PATH = jieba.dict.utf8 ---> 主词典（最重要！基础词库）
                里面存着几十万个中文标准词汇：
                中国、人民、我们、生活、技术、开发
                作用：基础分词全靠它。
            
            2. HMM_PATH = hmm_model.utf8 ---> HMM 模型（猜生词用）
                有些词不在字典里，比如：
                人名：张三峰
                新词汇：内卷、躺平、元宇宙
                网名：小草莓不甜
                HMM 模型靠算法猜出来这些词。

            3. USER_DICT_PATH = user.dict.utf8 ---> 用户自定义词典（公司业务词）
                这是你们公司自己加的专业词，比如：
                医疗：冠状动脉、CT 检查
                金融：私募基金、年化收益率
                游戏：王者荣耀、吃鸡、皮肤
                企业名：腾讯云、阿里云、抖音电商
                你想让分词识别你们行业词，就放这里。
    
            4. IDF_PATH = idf.utf8 ---> 关键词提取权重文件（提取重点词用）
                比如一句话：“我今天在公司使用C++开发了一个非常重要的算法模块”
                关键词提取会输出：C++、开发、算法、模块、公司
                靠 IDF 字典判断哪个词重要。

            5. STOP_WORD_PATH = stop_words.utf8 ---> 停用词（过滤没用的词）
                这些词没意义，分词后要扔掉：的、了、吗、呢、在、是、就、都、啊、呀、我们
                作用：减少干扰，让结果更干净。
    */
    

    //这里我们需要添加“字典路径常量” 
    //1.字典路径
    const char* const DICT_PATH="./dict/jieba.dict.utf8";
    const char* const HMM_PATH="./dict/hmm_model.utf8";
    const char* const USER_DICT_PATH="./dict/user.dict.utf8";
    const char* const IDF_PATH="./dict/idf.utf8";
    const char* const STOP_WORD_PATH="./dict/stop_words.utf8";

    // 下面我们来是现下一个类定义：“中文分词工具类：class JiebaUtil”
    class JiebaiUtil
    {
    private:
        /*
           这个 JiebaUtil 是中文分词工具类，它要干活，必须有两个东西：
                1. 能分词的机器 → jieba 对象
                1. 能过滤垃圾词的词典 → stop_words 停用词表
            所以类里一上来就定义这两个 “干活必备的家伙”

            1. 第一个成员：cppjieba::Jieba jieba; 它是什么？它就是真正的分词器本体！
            你可以理解成：JiebaUtil = 外壳、工具包装
            jieba = 里面真正干活的分词引擎，它必须写在这里的原因，分词必须靠它没有它，就不能分词。

            单例模式 = 全局只创建一次分词器因为加载字典很慢、很占内存。
            写在类里，全局唯一，只加载一次字典。
            它持有所有词典资源就是你刚才看到的：主词典、HMM 模型、用户词典……全都加载在这个 jieba 对象内部。
            一句话总结：cppjieba::Jieba jieba; = 分词工具的心脏。


            2. 第二个成员：std::unordered_map<std::string, bool> stop_words;
            它是什么？ 它是 “停用词词典” 的内存存储结构。
            停用词就是：的、了、吗、呢、在、是、啊、呀、我们、你们...

            为什么要存在类里？只加载一次，永久使用不用每次分词都读文件。
            查询速度极快 O (1)unordered_map 是哈希表，查一个词是不是垃圾词，瞬间出结果。
            分词后自动过滤垃圾分完词 → 查这个表 → 是垃圾词就扔掉。
            一句话总结：stop_words 是过滤垃圾词的过滤器。
        */
        // 1.结巴分词器的实例
        cppjieba::Jieba jieba; // 这里需要指定头文件#include "cppjieba/Jieba.hpp"

        // 2.停用词哈希表
        std::unordered_map<std::string, bool> stop_words; // 构建映射：key=停用词（UTF-8）---> value=占位符（true）

    private:
        /*  单例模式的实现：
                1. 私有构造函数
                2. 禁用拷贝构造函数
                3. 单例实例指针
        */
        // 1.实现：“私有构造函数”
        jieba() : jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH);

        // 2.实现：“禁用拷贝构造函数” --> 拷贝构造函数的一般格式：类名(const 类名& 引用名);
        JiebaUtil(const JiebaUtil &) = delete;

        // 3.单例实例指针
        static JiebaiUtil *instance;

        /* 这个单例模式到底在干嘛？为什么必须这么写？为什么要写成这样？
        一、先给你一句终极结论
            单例模式 = 保证整个程序里，永远只有一个 JiebaUtil 对象！绝对不允许出现第二个！

        二、为什么要单例？（最重要！）
            我给你讲最真实、最接地气的理由：
                1. 结巴分词加载字典超级慢、超级占内存！
                你看构造函数：
                    JiebaUtil() : jieba(
                        DICT_PATH,   // 主词典
                        HMM_PATH,    // 模型
                        USER_DICT_PATH,
                        IDF_PATH,
                        STOP_WORD_PATH
                    ){}
                    这 5 个文件加载：
                        慢：要读几十万个词
                        大：占用大量内存
                        不能重复加载：加载两次程序直接卡爆
                    如果不用单例，你写：JiebaUtil a;、JiebaUtil b;、JiebaUtil c;
                    那就会：加载 3 次字典 → 内存爆炸 → 程序卡死
                它实现了：全局只能有一个分词工具，谁都不能创建第二个！
                无论你在程序任何地方想用分词，拿到的永远是同一个对象、同一个分词器！

        四、我逐行解释：为什么代码要这么写？
            我带你一句一句看懂这三行核心代码。
        1️⃣ 构造函数私有化
            private:
                JiebaUtil() : jieba(...) {}
            作用：禁止外面随便创建对象！
            外面不能写：
            JiebaUtil a;   // 报错！构造函数是私有的！
            JiebaUtil b;   // 报错！
            为什么？
            防止别人乱创建对象 → 防止重复加载字典！

        2️⃣ 禁用拷贝构造
            JiebaUtil(const JiebaUtil&) = delete;
            作用：禁止复制对象！
            不能写：JiebaUtil a = JiebaUtil::getInstance();
            为什么？
            一复制就会创建第二个对象，就会加载第二遍字典！所以直接禁用！

        3️⃣ 静态实例指针
            static JiebaUtil* instance;
            作用：这是全局唯一的单例指针！整个程序只有这一个指针指向唯一的分词工具。
        */

    public:
        // 1.实现：“获取分词工具单例实例”
        static JiebaUtil *get_instance()
        {
            // 1.定义静态局部互斥锁
            static std::mutex mutex;
            /*
                为什么这里要用 静态局部互斥锁？
                答案一句话：因为这个函数是 static 静态成员函数，
                它里面的变量必须加 static 才能 “全局唯一、只创建一次”！

            1. 先记住一个铁律（超级重要）
                static 成员函数 = 没有 this 指针 = 不能用普通成员变量
                你这个函数：static JiebaUtil* get_instance() 是静态成员函数！
            它的特点：
                不属于某个对象
                属于整个类
                内部不能直接用普通的非静态变量
                想在里面用锁，必须用 static

            */

            /*
                情况 1：你以前写的代码（普通成员函数）
                class A {
                    std::mutex mtx; // 非静态锁

                    void func() {
                         mtx.lock(); // 有效！
                    }
                };
                ✅ 为什么有效？因为一个对象对应一把锁，多个线程访问同一个对象时，用的是同一把锁，所以能锁住。

                情况 2：现在的代码（静态成员函数）
                static JiebaUtil* get_instance()
                {
                    std::mutex mtx; // 非静态锁
                    mtx.lock();
                }

                ❌ 这把锁每次调用函数都会创建一个新的！
                    线程 A 调用 → 创建锁 A
                    线程 B 调用 → 创建锁 B
                    线程 C 调用 → 创建锁 C
                三把不同的锁，互相根本锁不住！
                这就等于：每个人自己带一把锁，锁自己的柜子根本不是共用一把锁！

                那为什么加 static 就对了？ static std::mutex mtx;
                加 static 后：
                    整个程序只创建一次
                    所有线程调用这个函数，用的都是同一把锁，真正能互斥！

            */

            // 2.外层判断单例对象到底有没有被创建出来 ---> 提升性能
            if (instance == nullptr)
            {
                // 2.1：加锁
                mutex.lock();

                // 2.2：内层判断单例对象到底有没有被创建出来 ---> 提升安全
                if (instance == nullptr)
                {
                    // 第一步：实例化单例对象
                    instance = new JiebaiUtil();
                    /* 看到这个形式的写法我们可以联想到“智能指针”：
                        智能指针 = new 构造函数();
                        单例实例指针 = new 构造函数();
                    */

                    // 第二步：初始化停用词表
                    instance->InitStopWord();
                }

                // 2.3：解锁
                mutex.unlock();
            }

            /*
            1. 为什么要写第一层 if 判断？
                    你想一个场景：单例对象已经创建好了，之后成千上万次调用 get_instance ()
                    如果没有第一层判断，代码会变成：
                        static JiebaUtil* get_instance()
                        {
                            static std::mutex mtx;
                            mtx.lock();       // 每次都加锁！
                            if (instance == nullptr) { ... }
                            mtx.unlock();
                            return instance;
                        }
                问题来了：加锁、解锁 是非常耗性能的！
                单例只需要创建 1 次，但你每次调用都 加锁 + 解锁，高并发场景下，程序会巨慢！

                加上第一层 if 后，发生了什么？
                if (nullptr == instance) {
                    // 只有第一次进来才会加锁
                }
                效果：
                    第一次创建：进 if，加锁，创建对象
                    后面 10000 次调用：instance 已经不为空，直接跳过 if，直接 return，完全不加锁！


            2. 第一层 if：为了快！不加锁，直接返回！那第二层 if 又是干嘛的？
                    你肯定会问：那里面还有一个 if 干嘛？
                    if (nullptr == instance) {  // 第一层：快
                        mtx.lock();
                        if (nullptr == instance) {  // 第二层：安全
                            instance = new ...
                        }
                        mtx.unlock();
                    }

                第二层 if 的作用：防止多线程同时创建多个对象！
                    比如：
                    线程 A → 判断 instance 为空 → 准备加锁
                    线程 B → 同时判断 instance 也为空 → 准备加锁
                    线程 A 先进去，创建对象
                    线程 B 后进去，必须再判断一次，否则会再创建一个！

        最清晰的总结（你一定记住）
            第一层 if：为了快！（不加锁，直接返回）
            第二层 if：为了安全！（防止多线程重复创建）

            */
            // 3.直接返回单例实例指针
            return instance;
        }



        // 2.实现：“初始化停用词表”
        void InitStopWord()
        {
            // 1.打开停用词文件
            std::ifstream in(STOP_WORD_PATH);
            if (in.is_open() == false)
            {
                std::cerr << "停用用词表初始化失败" << std::endl;
                return;
            }
            /*
                为什么可以这么写？
                你以前写的：in.open(路径, std::ios::in);
                和现在写的：std::ifstream in(路径);

                因为 std::ifstream 这个类自带构造函数：
                ifstream(const char* filepath); // ifstream 的构造函数之一
                意思就是：创建流对象的时候，直接把文件路径传进去，它会自动打开！

                默认以 文本模式（text mode）读取
                    也就是说：std::ifstream in(路径);
                    等价于：std::ifstream in(路径, std::ios::in);
            */

            // 2.逐行读取停用词文件中的内容
            std::string line;
            while (getline(in, line) == true)
            {
                // 将停用词添加哈希表中提升停用词的判断速度
                stop_words.insert(std::make_pair(line, true));
                // 或者写成：stop_words.insert({ line, true });
            }

            // 3.关闭文件
            in.close();
        }

        // 3.实现：“jieba分词的核心辅助函数”
        void CutStringHelper(const std::string &src, std::vector<std::string> *ret)
        {
            // 1.对字符串进行分词
            jieba.CutForSearch(src, *ret);

            // 2.遍历分词结果，过滤停用词
            for (auto it1 = ret.begin(), it1 != ret.end();)
            /*
                注意：这里 for 循环后面没有 iter++！因为删除元素时，迭代器会自己变化，不能随便 ++
            */
            {
                // 2.1：查询当前词是否是停用词
                auto it2 = stop_words.find(*it1);

                // 2.2：是停用词删除这个词并让迭代器指向删除后的下一个元素
                if (it2 != stop_works.end()) // 注意：这里的迭代器it2作用就是和停用词表打配合判断当前it1指向是词是不是停用词
                {
                    it1 = ret->erase(it1);
                }

                // 2.3：非停用词迭代器后移
                else
                {
                    it1++;
                }
            }
            /*
                终极结论（先背下来）
                    stop_words.find( 要找的内容 ) → 放的是要查找的 “值” → 所以必须用 *iter
                    out->erase( 要删的位置 )      → 放的是迭代器（位置） → 所以直接用 iter

            */
        }

    public:
        // 4.实现：“对外暴露分词接口函数”
        void CutString(const std::string &src, std::vector<std::string> *ret)
        {
            ns_util::JiebaUtil::get_instance()->CutstringHelper(src,ret);
        }
    };


    // ===================== 静态成员初始化 =====================

    //分词器单例实例指针初始化（全局唯一）
    JiebaUtil* JiebaUtil::instance = nullptr;

}

#endif 