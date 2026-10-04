#include "util.hpp"


//第一步：我们要定义一些全局的常量
//1.原始的HTML的根目录
const std::string src_path ="data/input";
//2.解析后文件输出的文件路径
const std::string out_path = "data/raw_html/raw.txt";


//第二步：我们需要定义一个文档信息结构体：存储的就是“一个HTML文档的核心的数据”
/*
    一个HTML文档的核心内容：
        1. 文档的标题
        2. 文档的内容
        3. 文档的url
*/
typedef struct DocInfo
{
    std::string title;
    std::string content;
    std::string url;
} DocInfo_t;


//第三步：实现一些核心的函数
//1.实现：“递归指定路径下的所有的HTML文件,并存储所有符合条件的HTML文件路径”
bool EnumFile(const std::string &src_path,std::vector<std::string> *file_list)
{
    //1.将字符串的文件路径变成boost库文件系统中路径对象
    boost::filesystem::path root_path(src_path);
    /*
        把一个字符串路径，变成一个 “路径对象”，方便操作文件 / 目录。
            1. 拆开来解释
                    boost::filesystem::path
                        这是 Boost 库专门用来表示 “文件路径” 的类作用：处理路径 拼接路径
                        判断是否是文件 / 目录
                        跨平台（Windows / Linux 通用）
                    root_path(src_path)
                        这里不是函数！是 创建一个 path 对象，名字叫 root_path用 src_path（字符串路径）来初始化它。
            2. 最直白翻译
                    boost::filesystem::path root_path(src_path);
                    等于：把字符串 src_path 变成一个路径对象 root_path
    */

    //2.检查文件路径是否存在
    if(!boost::filesystem::exists(root_path))
    /*
        exists用来判断 root_path 这个路径指向的东西，存在吗？
        严格来说：
            它不是类的成员函数
            它是 boost::filesystem 这个命名空间下的全局函数
        在 C++ 里，这种直接用命名空间调用、不需要对象的函数，
    */
    {
        std::cerr<<src_path<<"不存在"<<std::endl;
        return false;
    }


    //3.使用递归迭代器遍历path对象指向内容
    boost::filesystem::recursive_directory_iterator end;
    /*
        它是 Boost 库专门用来遍历目录 的高级工具！
            一句话终极解释：boost::filesystem::recursive_directory_iterator end;
            这是一个 “空的、结束标记” 迭代器，专门用来判断：目录遍历到头了没有！
        1. 先拆名字，你立刻懂
            recursive_directory_iterator = 递归 目录 迭代器
                recursive：递归（会自动进入子文件夹、孙子文件夹，全部遍历）
                directory：目录（文件夹）
                iterator：迭代器（相当于指针，用来挨个取文件）
                合起来：会自动钻进所有子文件夹的 文件遍历器
    */
    for(boost::filesystem::recursive_directory_iterator it(root_path), it!=end,it++)
    {
        //3.1：仅处理普通文件
        if(!boost::filesystem::is_regular_file(*it))
        {
            continue;
        }
        /* 作用：跳过文件夹、快捷方式、设备文件等所有 “不是普通文件” 的东西，只处理真正的文本 / 文档！
                1. 先拆函数：is_regular_file
                    boost::filesystem::is_regular_file(路径/迭代器)
                    判断：这个东西是不是【普通文件】
                    什么是普通文件？
                        就是你平时看到的：
                        .txt 文本
                        .html 网页
                        .jpg 图片
                        .cpp 代码
                    实实在在存数据的文件，什么 不是 普通文件？
                        文件夹（目录）
                        快捷方式
                        系统设备文件
                        管道、套接字
        */

        //3.2：处理带有.html后缀的文件
        if(it->path().extension()!=".html")
        {
            continue;
        }
        /*
            我拆成 4 段给你讲，超级简单
                1. it：递归目录迭代器对象相当于一个指向当前文件的指针

                2. it->path()：调用迭代器的 path() 方法拿到当前文件的完整路径对象 ./doc/测试.txt

                3. .extension()
                    调用路径对象的方法：拿文件后缀名
                    注意：返回的后缀 带点 .

                4. 合起来it->path().extension() = 当前文件的后缀名（带点）
        */

        //3.3：将符合条件的文件路径存入到容器中
        file_list->push_back(it->path().string());
        /*
            it->path()：拿到当前文件的 路径对象（boost::filesystem::path）
            .string()：把路径对象 → 转换成普通的 std::string 字符串
        */
    }

    //4.最后返回true
    return true;
}


//2.实现：“静态辅助函数 —— 从HTML文本中提取<title>标签内的标题”
static bool ParseTitle(const std::string &file,std::string *title)
//我们之前写：“递归指定目录下的所有HTML文件”的参数：1）文件的路径 2）存放合适文件路径的容器
//我们这里的“提取HTML文本中的标题”的参数：1）完整是HTML内容 2）这个HTML文档的标题
{
    //1.查找标签<title>的起始位置
    std::size_t begin=file.find("<title>")
    if(begin=std::string::npos)
    {
        return false;
    }

    //2.查找标签</tile> 的结束位置
    std::size_t end = file.find("</title>");
    if(end==std::string::npos)
    {
        return false;
    }

    ///3.起始迭代器偏移到<title>标签之后
    begin+=std::string("<title>").size();
    if(begin>end)
    {
        return false;
    }

    //4.截取文本中<title>标签内的标题
    *title=file.substr(begin,end-begin);
    return true
}


//3.实现：“静态函数 —— 去除HTML文件的所有的标签”
static bool ParseContect(const std::string file,std::string *content)
//之前静态成员函数：“提取<title>标签中的内容”，现在是去除HTML文件中所有的标签
//    参数：  1.完整的HTML文件的内容  2. 这个HTML中的纯文本内容
{
    //1.创建状态机枚举用来区分当前的字符是否的标签内
    enum status 
    {
        LABLE,
        CONTENT
    }

    //2.初始化状态默认在标签内
    enum staus s = LABLE;

    //3.遍历HTML文档中的每一个字符
    for(char c:file)
    {
        switch(s)
        {
            case LABLE:
                //情况一：在标签内 ---> 遇到符号'>'将状态改为CONTENT
                if(c=='>')
                {
                    s=CONTENT;
                }
                break;

            
            case CONTENT:
                //情况二：不在标签内 ---> 遇到符号'<'将状态改为LABLE
                if(c=='<')
                {
                    s=LABLE;
                }

                //情况三：不在标签内 ---> 遇到换行符'\n'的时候将其改成空格
                if(c=='\n')
                {
                    c=' ';
                    content->push_back(c);
                }
                break;

            default:
            break;
        }
    }

    //4.最后返回true
    return true;
}


//4.实现：“根据本地路径构建在线URL”
static bool ParseUrl(const std::string &file_path, std::string *url)
//根据以往的经验就是参数是： 1）一个HTML文档的完整的内容  2）提取出来url
//但是这个url构建是不是使用的一个HTML文档，而是“本地的HTML文档的路径” 
{
    //1.Boost官网文档根URL（对应1_78_0版本）
    std::string url_head = "https://www.boost.org/doc/libs/1_78_0/doc/html";

    //2.提取本地路径中src_path之后的相对路径（拼接为URL后缀）
    std::string url_tail = file_path.substr(src_path.size());
    /*
                src_path ="data/input";
                src_path.size()
    */


    //3.拼接完整URL
    *url = url_head + url_tail;
    return true;
}


//5.实现：“静态辅助函数 —— 打印单个文档的解析结果”
//参数九四待打印的文档信息结构体
static void ShowDoc(const DocInfo_t &doc)
{
    std::cout<<"文件的标题是"<<doc.title<<std::endl;
    std::cout<<"文件的内容是"<<doc.content<<std::endl;
    std::cout<<"文件的url是"<<doc.url<<std::endl;
}


//6.实现：“解析HTML文件列表，提取每个文件中的标题、内容、URL”
//参数说明：1）要解析文件文件列表   2）提取的文件内容可以使用文件信息结构体存储起来
bool ParseHtml(const std::vector<std::string> &file_list,DcoInfo_t *results)
{   
    //1.使用for循环遍历文件列表中的每一个文件路径
    for(const std::string &file_path:file_list)
    {
        //1.1：读取文件内容的到result字符串
        std::string result;
        bool ret = ns_util::FileUtil::ReadFile(file_path,&result);
        if(ret==false)
        {
            std::cerr<<"文件读取失败"<<std::endl;
            continue;
        }

        //2.定义文件信息结构体存储当前文件的解析结果
        DcoInfo_t doc;


        //2.提取文本标题
        if(!ParseTitle(result,&doc.title)
        {
            std::cerr<<"文件的题目解析失败"<<file_path<<std::endl;
            continue;
        }

        //3.提取文件内容
        if(!ParseContect(result,&doc.contect))
        {
            std::cerr<<"文件的内容解析失败"<<file_paht<<std::endl;
            continue;
        }

        //4.构建在线URL
        if(!ParseUrl(result,&doc.url))
        {
            std::cerr<<"构建文件的在线URL失败"<<file_path<<std::endl;
            continue;
        }

        //5.将文件信息结构体放入存储信息结构体的容器中
        results->push_back(std::move(doc));
        /*
            核心一句话：把 doc 里的内容 “转移 / 偷走” 给 vector，不拷贝数据，更快、更高效！

            1. 先讲最通俗的比喻
              假设：
                doc 是一个大箱子，里面装满了：
                    标题 title（一大段字符串）
                    内容 content（超级长的网页文本）
                    链接 url
                results 是一个大柜子，你要把箱子放进柜子里
                不用 move（传统拷贝）：
                    重新做一个一模一样的新箱子
                    把里面所有东西全部复制一遍放进柜子
                    原来的箱子还在，内容也还在 → 慢！浪费空间！浪费时间！
                用 move（转移）：
                    直接把原来的箱子搬进柜子
                    不复制任何东西
                    原来的箱子变空（内容被转移走了）→ 超快！零拷贝！效率极高！

                2. 代码层面的理解
                results->push_back( doc );
                这是拷贝：
                    vector 会复制一份 doc 的所有内容
                    原 doc 保持不变

                results->push_back( std::move(doc) );
                这是转移（移动）：
                    vector 直接拿走 doc 内部的数据
                    不复制字符串，不分配内存
                    doc 变成空的、无效的（之后不要再用它）


                3. 为什么这里可以用 move？
                看代码：
                DocInfo_t doc; // 创建局部变量

                // 给 doc 填充内容
                ParseTitle(...);
                ParseContent(...);
                ParseUrl(...);

                // 放进数组
                results->push_back(std::move(doc));

                // ！！！循环结束，doc 马上就要被销毁了！！！

                doc 是局部临时变量，
                这一行执行完，马上就要被销毁、扔掉了！
                既然都要扔了，为什么还要复制一遍？直接转移内容不是更快吗？
                所以：局部变量马上要销毁 → perfect 使用 std::move 的场景！


                4. 最关键的结论（背这句）
                std::move(doc)
                = 告诉编译器：我不用这个 doc 了，你可以把它的内容 “转移” 走！
                = 不拷贝、更快、更高效！
        */

    }

    //2.只要有一个HTML文档解析成功就返回true
    return !results.empty()
}


//7.实现：“解析后文档信息写入输出文件”
/*
    这个时候我们就能发现：三个主要函数之间的关系串起来了
    1. EnumFile  (string ---> vector<string>)
    2. ParseHTML (vector<strig> ---> vector<DocInfo_t>)
    3. SaveHTML  (vector<DocInfo_t> ---> string)
*/
bool SaveHtml(const vector<DocInfo_t> &results,std::string *out_path)
{
    //1.定义字段分隔符
#define DEP '\3'

    //2.以二进制的模式打开输出文件
    std::ofstream out(*out_path,std::ios::out|std::ios::binary);
    if(!out.is_open())
    {
        std::cerr<<*out_path<<"文件打开失败"<<std::endl;
        return false;
    }

    //3.遍历所有解析结果逐行写入
    for(auto& item:results)
    {
        //2.1：定义字符串存储拼接的单行内容
        std::string out_string;

        //2.2：拼接单行输出内容（title+分隔符+content+分隔符+url+换行）
        out_string +=item.title;
        out_string +=SEP;
        out_string +=item.content;
        out_string +=SEP;
        out_string +=item.url;
        out_string +='\n';

        //2.3：将单行内容写入到文件中
        out.write(out_string.c_str(),out_string.size()); //c_str()获取字符串首地址，size()获取字节数
        /*
            out：输出文件流（std::ofstream），专门用来写文件的对象
            → 相当于钢笔

            write( )：out 的成员函数，二进制写入 / 原始字符串写入
            → 作用：把一段内存数据直接写到文件里

            out_string.c_str()：获取字符串内部的 C 风格字符指针（const char）*
            → 要写入的数据的起始地址

            out_string.size()：字符串长度
            → 要写入多少字节
        */
    }

    //4.关闭文件
    out.close()

    //5.释放宏定义并返回true
    #undef SEP
    return true;
}

//最后实现：“主函数”——完成：“枚举文件 -> 解析内容 -> 保存内容”
int main()
{
    //1.枚举文件
    std::vector<std::string> file_list;
    if(!EnumFile(src_path,&file_list))
    {
        std::cerr<<"文件枚举失败";
        return 1;
    }

    //2.解析内容
    std::vetor<DocInfo_t> results;
    if(!ParseHTML(file_list,&results))
    {
        std::cerr<<"文件解析失败"<<std::endl;
        return 2;
    }

    //3.保存内容
    if(!SaveHtml(results,out_put))
    {
        std::cerr<<"文件保存失败"<<std::endl;
        return 3;
    }
    return 0;
}