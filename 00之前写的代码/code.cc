#include <iostream>       // 提供“C++标准输入输出流”如：cout
#include <ctype.h>        // 提供“字符处理函数”如：isspace
#include <cstdio>         // 提供“C标准输入输出函数”如：printf、fgets、fflush
#include <cstring>        // 提供“字符串处理函数”如：strlen、strcpy、strtok
#include <cstdlib>        // 提供“内存分配、进程退出等函数”如：malloc、free、exit、putenv
#include <unistd.h>       // 提供“Unix系统调用”如：fork、execvp、chdir、getcwd、getenv
#include <sys/types.h>    // 提供“系统数据类型定义”如：pid_t，进程ID类型
#include <sys/wait.h>     // 提供“进程等待函数”如：waitpid、WEXITSTATUS
#include <unordered_map>  // 提供“无序映射容器”
#include <sys/stat.h>     // 提供“文件状态相关定义”如：open函数依赖
#include <fcntl.h>        // 提供“文件打开选项”如：O_RDONLY、O_CREAT等

//1.命令行缓冲区大小：限制单次输入命令的最大长度
#define COMMAND_SIZE 1024
//2.命令行提示符格式：[用户名@主机名 当前目录]# 
#define FORMAT "[%s@%s %s]# "



/*------------------------------------------ 全局数据定义 ------------------------------------------*/
//1.命令行参数表：存储解析后的命令及参数（如："ls -a -l"解析为["ls", "-a", "-l", NULL]）
#define MAXARGC 128     // 最大参数个数
char* g_argv[MAXARGC];  // 参数数组（每个元素是参数字符串指针）
int g_argc = 0;         // 实际参数个数（不含末尾的NULL）

//2.环境变量表：存储Shell管理的环境变量（如：PATH、PWD等）
#define MAX_ENVS 100    // 最大环境变量个数
char* g_env[MAX_ENVS];  // 环境变量数组（每个元素是"变量名=值"字符串指针）
int g_envs = 0;         // 实际环境变量个数（不含末尾的NULL）


//3.测试用变量：存储当前工作目录及对应的环境变量字符串（PWD=路径）
char cwd[1024];     // 存储getcwd获取的当前工作目录
char cwdenv[2048];  // 存储"PWD=当前目录"格式的字符串，用于putenv设置环境变量

//4. 上一条命令的退出码：0表示成功，非0表示失败（对应Shell中的$?）
int lastcode = 0;


//5.别名映射表：存储命令别名（如："ll"映射为"ls -l"）
std::unordered_map<std::string, std::string> alias_list;

//6.重定向相关定义：标记重定向类型及目标文件
#define NONE_REDIR 0   // 无重定向
#define INPUT_REDIR 1  // 输入重定向（<）：从文件读取输入，而非标准输入
#define OUTPUT_REDIR 2 // 输出重定向（>）：将输出写入文件，覆盖原有内容
#define APPEND_REDIR 3 // 追加重定向（>>）：将输出追加到文件末尾
//7.当前命令的重定向类型（默认无重定向）
int redir = NONE_REDIR;  
//8.重定向目标文件名（如："file.txt"）
std::string filename;    



/*------------------------------------------ 工具函数实现 ------------------------------------------*/
//1.获取当前用户名（从USER环境变量中读取）
const char* GetUserName()
{
    const char* name = getenv("USER");   // getenv：从环境变量中获取指定变量的值
    return name == NULL ? "None" : name; // 若未获取到，返回"None"
}

//2.获取当前主机名（从HOSTNAME环境变量中读取）
const char* GetHostName()
{
    const char* hostname = getenv("HOSTNAME");
    return hostname == NULL ? "None" : hostname;
}

//3.获取当前工作目录
//（注意：优先从getcwd获取，同步更新PWD环境变量，所以这里我们没有使用getenv("PWD")的方式获取当前的工作目录）
const char* GetPwd()
{
    //3.1：使用getcwd获取当前进程的工作目录，存储到cwd缓冲区
    const char* pwd = getcwd(cwd, sizeof(cwd));

    //3.2：若成功获取工作目录，则更新PWD环境变量
    if (pwd != NULL)
    {
        //第一步：构建"PWD=当前目录"格式的环境变量字符串
        snprintf(cwdenv, sizeof(cwdenv), "PWD=%s", cwd);

        //第二步：使用putenv将字符串添加到当前进程的环境变量中
        putenv(cwdenv); 
        //注意：putenv函数会更新当前进程的环境变量表，使PWD的值与实际工作目录一致
    }
    //3.3：返回获取结果
    return pwd == NULL ? "None" : pwd;

    //注意：getcwd是获取当前工作目录的系统调用函数，主要作用是获取进程当前所在的绝对路径，并存储到指定缓冲区中
    /* 参数说明：
    *   第一个参数（char* buf）：用于存储工作目录路径的缓冲区（如：cwd）
    *   第二个参数（size_t size）：缓冲区的最大容量（如：sizeof(cwd)）
    *                              确保能容纳完整路径（包括结束符'\0'）
    *
    *   返回值说明：
    *     成功：返回指向缓冲区的指针（与第一个参数相同）
    *     失败：返回NULL（如：路径长度超过缓冲区大小、权限不足等）
    *
    *   注意：getcwd获取的是绝对路径，反映进程当前的工作目录（可通过cd命令改变）
    */
}



//4.获取用户家目录（从HOME环境变量中读取）
const char* GetHome()
{
    const char* home = getenv("HOME");
    return home == NULL ? "" : home; // 若未获取到，返回空字符串
}


//5.从完整路径中提取最末级目录名（如："/a/b/c"提取"c"，"/"返回"/"）
std::string DirName(const char* pwd)
{
    //5.1：定义路径分隔符为"/"
#define SLASH "/"

    //5.2：将C风格字符串（const char*）转换为C++ string，方便后续字符串操作
    std::string dir = pwd;

    //5.3：特殊情况处理：如果路径本身就是根目录"/"，直接返回"/"
    if (dir == SLASH) return SLASH;

    //5.4：从字符串末尾查找最后一个路径分隔符"/"
    auto pos = dir.rfind(SLASH);

    //5.5：如果未找到任何"/"（理论上合法路径至少包含根目录"/"）
    if (pos == std::string::npos) return "BUG?";

    //5.6：截取其后的字符串（即目录名）
    return dir.substr(pos + 1);
    //注意：从最后一个"/"的下一个位置开始截取子字符串，得到最末级目录名
}




//6.构建命令行提示符
void MakeCommandLine(char cmd_prompt[], int size)
{
    //注意：格式化提示符为 [用户名@主机名 目录名]#
    snprintf
    (
        cmd_prompt,       //1.用于存储生成的提示符字符串的缓冲区
        size,             //2.缓冲区的最大容量（防止字符串溢出）   
        FORMAT,           //3.格式字符串，定义为"[%s@%s %s]# "

        GetUserName(),    //4.替换格式中的第一个%s，填入用户名
        GetHostName(),    //5.替换格式中的第二个%s，填入主机名
        DirName(GetPwd()).c_str() //6.替换格式中的第三个%s，填入当前目录名（字符串需转换为C风格）
    );
    //注意：snprintf主要作用是将格式化的字符串写入指定缓冲区，并确保不会超出缓冲区的最大长度，从而避免缓冲区溢出问题

    /* snprintf参数说明：
    *   参数1（str）：目标缓冲区指针，用于存储格式化后的字符串
    *   参数2（size）：缓冲区的最大字节数（包含结束符'\0'），若格式化结果超长，会截断为size-1个字符并自动添加'\0'
    *   参数3（format）：格式化字符串，包含普通字符和格式占位符（如：%s、%d等）
    *          后续可变参数：按顺序替换format中的占位符，类型需与占位符匹配（如：%s对应字符串，%d对应整数）
    *
    *   返回值：若成功，返回理论上应写入的字符总数（不含'\0'）；若发生截断，返回值大于等于size
    */
}


//7.去除字符串末尾的空白字符（用于重定向解析时清理文件名前的空格）
void TrimSpace(char cmd[], int& end)
{
    //7.1：循环跳过所有空白字符（包括空格、制表符、换行符等）
    while (isspace((unsigned char)cmd[end]))
    {
        end++; // 跳过当前空白字符，end索引后移
    }
    //注意：isspace其核心作用是：判断一个字符是否为“空白字符”
    /* isspace的参数解析：
    *      函数原型：int isspace(int c);
    *      参数c：需检测的字符，以int类型传递（实际为字符的ASCII码值）
    *           注意：必须传入unsigned char类型转换后的字符或EOF，
    *                 否则当字符为负数（如扩展ASCII中的某些字符）时，可能导致未定义行为
    *      返回值：若字符是空白字符（空格' '、制表符'\t'、换行符'\n'、回车符'\r'、垂直制表符'\v'、换页符'\f'）
    *             返回非0值（真）；否则返回0（假）
    * ********************************************************************************************************
    *  此处用于跳过重定向符号（如：>、>>、<）与目标文件名之间的空白字符，
    *  确保后续能正确提取文件名（如：将" >>  output.txt"中的end从空格位置移至'o'的位置）
    */
}

/*------------------------------------------ 环境变量初始化 ------------------------------------------*/
// 初始化Shell的环境变量表：从系统environ复制，并添加自定义环境变量
void InitEnv()
{
    //1.声明外部全局变量environ：指向系统环境变量数组
    extern char** environ;  

    //2.初始化环境变量数组为0（清空）
    memset(g_env, 0, sizeof(g_env)); 
    g_envs = 0;

    //3.从系统environ复制环境变量到Shell的g_env数组
    for (int i = 0; environ[i]; i++)
    { 
        //第一步：为每个环境变量分配内存（+1预留'\0'空间）
        g_env[i] = (char*)malloc(strlen(environ[i]) + 1);
        //注意：environ是系统全局变量，指向进程的环境变量数组（每个元素为"变量名=值"格式的字符串）

        //第二步：将系统环境变量的内容复制到Shell自己的环境变量数组中
        strcpy(g_env[i], environ[i]); 
        /* 注意：
        *    1. strcpy会自动复制包括结束符'\0'在内的整个字符串
        *    2. 此时g_env[i]与environ[i]内容相同，但存储在不同的内存地址（Shell独立管理）
        */

        //第三步：更新Shell环境变量的计数（记录当前已复制的环境变量数量）
        g_envs++;
    }
    //4.添加自定义环境变量（测试用）
    g_env[g_envs++] = (char*)"HAHA=for_test";
    g_env[g_envs] = NULL; //注意：环境变量数组以NULL结尾（标准要求）





    //5.将g_env中的环境变量添加到当前进程的环境中
    for (int i = 0; g_env[i]; i++)
    {
        putenv(g_env[i]);
    }
    //注意：这一步确保子进程（如：Shell 执行的外部命令）能继承这些环境变量

    //6.让系统environ指向Shell的g_env数组（统一环境变量管理）
    environ = g_env;
    /* 作用：将系统全局的 environ 指针（默认指向系统环境变量数组）重新指向 Shell 自己的 g_env 数组。
    *
    *  意义：
    *    1. 后续对环境变量的修改（如：export 命令）只需操作 g_env 数组，系统会自动识别更新（因为 environ 已指向它）
    *    2. 实现了 Shell 对环境变量的 “统一管理”，避免系统环境与 Shell 环境不一致的问题
    */
}

/*------------------------------------------ 内置命令实现 ------------------------------------------*/
//1.实现cd命令：切换工作目录
bool Cd()
{
    //1.1：保存当前工作目录，用于更新OLDPWD环境变量（记录上一次目录）
    char old_pwd[1024];

    //1.2：获取切换前的目录
    getcwd(old_pwd, sizeof(old_pwd));  

    //1.3：若cd无参数（argc=1），默认切换到家目录
    if (g_argc == 1)
    {
        //第一步：获取家目录路径（从HOME环境变量）
        std::string home = GetHome();  

        //第二步： 家目录为空（异常情况），直接返回
        if (home.empty()) return true; 

        //第三步：调用chdir系统调用切换工作目录
        chdir(home.c_str());
        //注意：chdir改变当前进程的工作目录，后续的文件操作（如：打开文件）会以新目录为基准

        /* chdir的参数说明：
        *   参数：const char* path - 目标目录的路径字符串（可以是绝对路径或相对路径）
        *        例如："/home/user"（绝对路径）、"../docs"（相对路径）
        *
        *   返回值：int类型，成功返回0，失败返回-1（并设置errno表示错误原因）
        *
        *   注意：该调用只影响当前进程，不会影响父进程（如启动Shell的进程）
        */
    }


    //1.4：若cd有一个参数及以上（argc=2）
    else
    {
        //第一步：获取目标目录参数（第二个参数）
        std::string where = g_argv[1];  

        //情况一：
        if (where == "-")
        {
            //处理cd -：切换到上一次工作目录（依赖OLDPWD环境变量）
            //1）从环境变量获取上一次目录
            const char* old_dir = getenv("OLDPWD");  

            //2）若OLDPWD存在
            if (old_dir)  
            {
                chdir(old_dir); 
                
                printf("%s\n", old_dir);
            }
        }
        //情况二：
        else if (where == "~")
        {
            //处理cd ~：切换到家目录（与无参数cd行为一致）
            std::string home = GetHome();
            if (!home.empty())
            {
                chdir(home.c_str());
            }
        }
        //情况三：
        else
        {
            //处理指定目录：切换到绝对路径或相对路径表示的目录
            chdir(where.c_str());
        }
    }

    return true;
}




//2.实现echo命令：打印参数或环境变量
void Echo()
{
    if (g_argc == 2)  
    {
        //2.1：将参数转换为字符串，便于后续判断和处理
        std::string opt = g_argv[1];

        //情况1：参数为"$?"，打印上一条命令的退出码
        if (opt == "$?")
        {
            // 第一步：输出全局变量lastcode存储的上一条命令退出码
            std::cout << lastcode << std::endl;

            //第二步：打印后重置退出码（符合Shell行为，避免影响后续命令）
            lastcode = 0; 
        }

        //情况2：参数以"$"开头（且不是"$?"），视为环境变量引用（如：$PATH、$USER）
        else if (opt[0] == '$')
        {
            //第一步：截取"$"后面的部分作为环境变量名（如："$PATH"截取为"PATH"）
            std::string env_name = opt.substr(1); 

            //第二步：调用getenv获取环境变量的值
            const char* env_value = getenv(env_name.c_str());

            //第三步：若环境变量存在，则打印其值
            if (env_value)  std::cout << env_value << std::endl;
        }

        //情况3：普通字符串参数（如："hello"、"123"），直接打印
        else
        {
            // 直接打印普通字符串（如：echo "hello"）
            std::cout << opt << std::endl;
        }
    }
}

//3.实现export命令：设置或显示环境变量（内置命令，影响当前Shell及子进程继承的环境）
void Export()
{
    //情况一：无参数（export）---> 显示所有已定义的环境变量
    if (g_argc == 1)
    {
        //1.1：遍历全局环境变量数组g_env，依次打印每个环境变量（格式：变量名=值）
        for (int i = 0; g_env[i]; i++)
        {
            std::cout << g_env[i] << std::endl;
        }
    }

    //情况二：带参数（export 变量名=值 或 export 变量名）---> 处理环境变量的设置或查询
    else if (g_argc >= 2)
    {
        //2.1：获取第一个参数（环境变量表达式，如："PATH=/usr/local/bin"或"PATH"）
        std::string env_var = g_argv[1];

        //2.2：查找字符串中"="的位置，用于区分"变量名=值"和单纯"变量名"两种格式
        size_t pos = env_var.find('=');

        //子情况1：参数中包含"="（格式：export NAME=value）---> 设置新的环境变量
        if (pos != std::string::npos)
        {
            //1)检查环境变量数组是否还有空间（预留最后一个位置存储NULL作为结束标志）
            if (g_envs < MAX_ENVS - 1)
            {
                //1.1)为新环境变量分配内存（+1用于存储字符串结束符'\0'）
                g_env[g_envs] = (char*)malloc(env_var.length() + 1);

                //1.2)将环境变量字符串（如："NAME=value"）复制到数组中
                strcpy(g_env[g_envs], env_var.c_str());

                //1.3)环境变量计数+1
                g_envs++;

                //1.4)确保数组以NULL结尾（符合环境变量数组的标准格式）
                g_env[g_envs] = NULL;

                //1.5)调用putenv更新当前进程的环境变量表，使新设置的变量能被子进程继承
                putenv(g_env[g_envs - 1]); //注意：传入的参数是刚添加到g_env的环境变量字符串（g_envs-1是其索引，因为刚刚将g_envs进行了++操作）
            }

        }

        //子情况2：参数中不包含"="（格式：export NAME）---> 显示指定环境变量的值
        else
        {
            //2.1)调用genv从环境变量表中查询指定变量的值
            const char* value = getenv(env_var.c_str());

            //2.2)若变量存在（value != NULL），则按"变量名=值"的格式打印
            if (value)
            {
                std::cout << env_var << "=" << value << std::endl;
            }
        }
    }
}


//4.实现alias命令：管理命令别名（支持定义、查询和显示所有别名，如：alias ll='ls -l'）
void Alias()
{
    //情况1：无参数（仅输入alias）→ 显示所有已定义的别名
    if (g_argc == 1)
    {
        //1.1：遍历全局别名映射表alias_list（key为别名，value为对应的命令）
        for (const auto& alias : alias_list)
        {
            //注意：按标准格式"alias 别名='命令'"打印，如："alias ll='ls -l'"
            std::cout << "alias " << alias.first << "='" << alias.second << "'" << std::endl;
        }
    }

    //情况2：带参数（alias 别名=命令 或 alias 别名）→ 定义新别名或查询单个别名
    else if (g_argc >= 2)
    {
        //2.1：获取第一个参数（别名表达式，如："ll='ls -l'"或"ll"）
        std::string arg = g_argv[1];

        //2.2：处理参数被空格分割的情况（如：用户输入alias ll='ls -l -a'时，参数可能被拆分为多个）
        if (g_argc > 2) //注意：当参数数量超过2个时，需要将后续参数重新拼接成完整的命令字符串
        {
            for (int i = 2; i < g_argc; i++)
            {
                //第一步：用空格分隔参数片段
                arg += " ";        

                //第二步：拼接后续参数 
                arg += g_argv[i];  
            }
        }
        /* 上面的这两步有小伙伴可能看不懂是什么意思，这里我们来梳理一下：
        *       当用户输入包含空格的别名定义时，比如：alias ll='ls -l -a'
        *  
        * 在命令行解析时，参数可能会被拆分为多个部分（取决于解析逻辑），此时：
        *       1. g_argv[0] 是命令名 alias
        *       2. g_argv[1] 是第一个参数 ll='ls
        *       3. g_argv[2] 是第二个参数 -l
        *       4. g_argv[3] 是第三个参数 -a'
        *       5. g_argc 的值为 4（总参数数量）
        * 最终拼接后的 arg 为完整的别名表达式 "ll='ls -l -a'"，后续代码就能正确提取别名 ll 和对应的命令 ls -l -a
        */

        //2.3：查找参数中"="的位置，区分"定义别名"和"查询别名"两种操作
        size_t pos = arg.find('=');


        //子情况2.1：参数中包含"="（格式：alias name='command'）→ 定义新别名
        if (pos != std::string::npos)
        {
            //第一步：提取别名名称（=左侧的字符串，如："ll='ls -l'"中的"ll"）
            std::string name = arg.substr(0, pos);

            //第二步：提取别名对应的命令（=右侧的字符串，如："ll='ls -l'"中的"'ls -l'"）
            std::string command = arg.substr(pos + 1);

            //第三步：处理命令两端可能存在的单引号或双引号（如：用户输入alias ll="ls -l"）
            if (command.length() >= 2 &&
                ((command[0] == '\'' && command[command.length() - 1] == '\'') ||
                    (command[0] == '"' && command[command.length() - 1] == '"')))
                //条件：命令长度至少为2，且首尾字符都是单引号或都是双引号
            {
                // 去除首尾引号，如："'ls -l'"→"ls -l"，"\"ls -l\""→"ls -l"
                command = command.substr(1, command.length() - 2);
            }

            //第四步：将别名和对应的命令存入映射表（若别名已存在则覆盖原有定义）
            alias_list[name] = command;
        }

        //子情况2.2：参数中不包含"="（格式：alias name）→ 查询单个别名的定义
        else
        {
            //第一步：在别名映射表中查找指定的别名
            auto it = alias_list.find(arg);

            //第二步：若找到该别名（it != alias_list.end()）
            if (it != alias_list.end())
            {
                // 按标准格式打印该别名的定义，如"alias ll='ls -l'"
                std::cout << "alias " << it->first << "='" << it->second << "'" << std::endl;
            }

            //第三步：若未找到该别名
            else
            {
                // 提示该别名不存在，如："alias: ll: not found"
                std::cout << "alias: " << arg << ": not found" << std::endl;
            }
        }
    }
}


/*------------------------------------------ 核心流程函数 ------------------------------------------*/

//1.打印命令行提示符
void PrintCommandPrompt()
{
    //1.1：定义一个字符数组用来存储提示符字符串
    char prompt[COMMAND_SIZE];

    //1.2：生成提示符字符串
    MakeCommandLine(prompt, sizeof(prompt));  

    //1.3：打印命令行提示符
    printf("%s", prompt);  
    
    //1.4：强制刷新输出（确保提示符立即显示）
    fflush(stdout);                         
}

//2.获取用户输入的命令行
bool GetCommandLine(char* out, int size)
{
    //2.1：从标准输入读取一行命令（包含换行符'\n'）
    //注意细节：fgets会读取包括换行符'\n'在内的字符，最多读取size-1个字符（预留'\0'位置）
    char* c = fgets(out, size, stdin);

    //2.2：读取失败（如：用户输入EOF，通常是Ctrl+D）
    if (c == NULL) return false;  

    //2.3：去除末尾的换行符（替换为字符串结束符'\0'）
    out[strlen(out) - 1] = 0;

    //2.4：空命令（仅输入回车）返回false
    if (strlen(out) == 0) return false;

    return true;
    //注意：fgets主要作用是从指定的输入流（如：标准输入stdin、文件等）中读取一行字符串，并存储到目标缓冲区中。
    /* fgets参数说明：
    *       参数一（char* str）：指向存储读取结果的字符数组（缓冲区）
    *       参数二（int size）：缓冲区的最大容量（字节数），最多读取size-1个字符
    *       参数三（FILE* stream）：输入流指针，stdin表示标准输入（键盘），也可传入文件指针读取文件
    *  返回值说明：
    *       成功读取时，返回指向缓冲区的指针（与第一个参数相同）
    *       读取失败或到达文件末尾（EOF）时，返回NULL
    */
}


//3.重定向分析：从命令行字符串中提取“重定向类型”（输入/输出/追加）和“目标文件名称”
//注意：参数 cmd[]：原始命令行字符串（例如："ls > log.txt" 或 "cat < input.txt"）
void RedirCheck(char cmd[])
{
    /*-------------------------准备阶段-------------------------*/
    //3.1：重置全局重定向类型标识为"无重定向"
    redir = NONE_REDIR;

    //3.2：重置全局目标文件名（清空之前可能存储的文件名）
    filename.clear();

    //3.3：命令行字符串的起始索引（辅助判断边界）
    int start = 0;


    /*-------------------------分析阶段-------------------------*/
    //3.4：从命令行字符串的末尾开始查找重定向符号（<、>、>>）
    //注意：从末尾查找的原因：重定向符号通常出现在命令尾部（如："cmd > file"）
    int end = strlen(cmd) - 1;
    while (end > start)
    {
        //情况1：找到输入重定向符号 '<'（例如："cmd < input.txt"）
        if (cmd[end] == '<')
        {
            //第一步：将 '<' 替换为字符串结束符 '\0'，截断命令部分（例如："cmd < input.txt" → "cmd "）
            cmd[end++] = 0; //注意：这里 end++ 是为了让后续文件名从符号后开始提取

            //第二步：跳过文件名前的空白字符（例如：处理 "cmd <  input.txt" 中的空格，使 end 指向 'i'）
            TrimSpace(cmd, end);

            //第三步：将重定向类型标记为"输入重定向"
            redir = INPUT_REDIR;

            //第四步：记录目标文件名（从 end 位置开始的字符串）
            filename = cmd + end;

            //第五步：找到重定向符号后退出循环
            break;
        }

        //情况2：找到输出相关的重定向符号 '>'
        else if (cmd[end] == '>')
        {
            //子情况2.1：连续两个 '>'，即追加重定向 ">>"（例如："cmd >> log.txt"）
            if (cmd[end - 1] == '>')
            {
                cmd[end - 1] = 0;     // 将第一个 '>' 替换为结束符，截断命令部分（例如："cmd >> log.txt" → "cmd "）
                redir = APPEND_REDIR; // 标记为"追加重定向"
            }
            //子情况2.2：单个 '>'，即输出重定向 ">"（例如："cmd > log.txt"）
            else
            {
                redir = OUTPUT_REDIR; // 标记为"输出重定向"
            }

            //2.3：将当前的 '>' 替换为结束符，进一步截断（确保命令部分不包含符号）
            cmd[end++] = 0;

            //2.4：跳过文件名前的空白字符（例如：处理 "cmd >  log.txt" 中的空格）
            TrimSpace(cmd, end);

            //2.5：记录目标文件名
            filename = cmd + end;

            //2.6：找到重定向符号后退出循环
            break;
        }

        //情况3：既不是 '<' 也不是 '>'，继续向前遍历
        else
        {
            end--;
        }
    }
    /* 函数执行完毕后：
    *     1. redir 变量存储重定向类型（或 NONE_REDIR）
    *     2. filename 存储目标文件名（若有重定向）
    *     3. cmd 字符串被截断为纯命令部分（不含重定向符号和文件名）
    */
}

//4.命令行解析：将输入字符串拆分为参数数组
bool CommandParse(char* commandline)
{
//4.1：定义参数分隔符为空格
#define SEP " "  //注意：这里要使用双引号的空格而不能是单引号，原因是strtok的第二个参数的类型是const char*
    //4.2：重置全局参数计数器，准备存储新的解析结果
    g_argc = 0;  

    //4.3：第一次调用strtok：分割第一个参数（命令名）
    g_argv[g_argc++] = strtok(commandline, SEP);

    //4.4：继续分割剩余参数，直到strtok返回NULL
    while ((bool)(g_argv[g_argc++] = strtok(nullptr, SEP)));
    //注意：将赋值表达式的结果（子串指针）转换为布尔值 —— 非 NULL 指针转换为 true，NULL 转换为 false

    //4.5：减去最后一个NULL的计数
    g_argc--;  

    //4.6：若存在有效参数则返回true
    return g_argc > 0;  

    //注意：strtok主要作用是按照指定的分隔符（如：空格、逗号等）将一个字符串拆分为多个子串（称为“标记”）

    /* strtok参数说明：
    *   第一个参数（char* str）：
    *       首次调用时：传入待分割的原始字符串（如：commandline）
    *       后续调用时：传入NULL，表示继续分割上一次的字符串
    *   第二个参数（const char* delim）：
    *     分隔符集合字符串（如SEP定义的" "，表示空格为分隔符）
    *
    *   返回值说明：
    *       成功找到子串时：返回指向该子串的指针（子串以'\0'结尾）
    *       无更多子串时：返回NULL
    *
    *   注意：strtok会直接修改原始字符串（将分隔符替换为'\0'），因此不能用于常量字符串
    */
}

//5.检测并替换别名：在执行命令前检查是否存在别名，若存在则替换为实际命令
bool CheckAndReplaceAlias() //功能：例如将"ll"替换为"ls -l"，并保留原始参数（如："ll -a"→"ls -l -a"）
{
    //5.1：若命令参数为空（无有效命令），直接返回false（无需替换）
    if (g_argc == 0) return false;

    //5.2：提取命令名（即第一个参数，如："ll"是命令名）
    std::string cmd = g_argv[0];
    //5.3：在别名映射表（alias_list）中查找该命令是否有对应的别名
    auto it = alias_list.find(cmd);

    //5.4：若找到对应的别名（it != alias_list.end()表示查找成功）
    if (it != alias_list.end())
    {
        //第一步：构建新的命令字符串，初始化为别名对应的实际命令（如："ls -l"）
        std::string new_cmd = it->second;

        //第二步：将原始命令中的参数追加到新命令后（保留用户输入的参数）
        // 例如：原始命令"ll -a"中，"-a"是参数，需追加到"ls -l"后→"ls -l -a"
        for (int i = 1; i < g_argc; i++)  //注意：从索引1开始遍历（跳过命令名）
        {
            //1)参数间用空格分隔
            new_cmd += " ";    

            //2)追加原始参数（如："-a"） 
            new_cmd += g_argv[i];         
        }

        /************************************************* 方法一：使用动态缓冲区 ********************************************************/
        ////第三步：为新命令字符串分配临时缓冲区（+1预留结束符'\0'的空间）
        //char* cmd_buf = new char[new_cmd.length() + 1];

        ////第四步：将新命令字符串复制到缓冲区（转换为C风格字符串）
        //strcpy(cmd_buf, new_cmd.c_str());

        ////第五步：调用CommandParse重新解析替换后的命令
        //// 作用：更新全局参数数组g_argv和参数计数g_argc，使后续执行使用替换后的命令
        //CommandParse(cmd_buf);

        //////第六步：释放临时缓冲区（避免内存泄漏）
        ////delete[] cmd_buf;

        ///* 注意：千万不要添加到第六步的内容：
        //*       1. CommandParse() 函数使用 strtok 修改原始字符串并设置 g_argv 指向该字符串的各个部分
        //*       2. 但您立即释放了缓冲区，导致 g_argv 指向了已释放的内存
        //*/
        /************************************************* 方法一：使用静态缓冲区 ********************************************************/


        //第三步：使用静态缓冲区静态缓冲区来存储替换后的命令字符串
        //注意：使用static关键字确保缓冲区在函数调用间保持存在，避免因函数返回而被释放
        static char alias_buffer[COMMAND_SIZE];

        //第四步：将替换后的命令字符串（new_cmd）复制到静态缓冲区中
        //注意：strncpy确保只复制指定长度的内容，参数sizeof(alias_buffer) - 1预留最后一个位置给终止符
        strncpy(alias_buffer, new_cmd.c_str(), sizeof(alias_buffer) - 1);

        //第五步：强制在缓冲区末尾添加终止符'\0'，确保字符串完整（即使new_cmd长度超过缓冲区大小）
        alias_buffer[sizeof(alias_buffer) - 1] = '\0';

        //第六步：调用命令解析函数重新解析替换后的命令
        //注意：将静态缓冲区中的完整命令字符串传入，让系统按照新命令执行，例如：原本输入"ll"，替换为"ls -l"后，此处会解析执行"ls -l"
        CommandParse(alias_buffer);
        /**********************************************************************************************************************************/

        //第七步：返回true表示别名替换成功，告知调用方命令已被替换并重新解析
        return true;
    }

    //5.5：若未找到对应的别名，返回false
    return false;
}



//6.执行内置命令（内置命令由Shell直接执行，无需创建子进程）
bool CheckAndExecBuiltin()
{
    //6.1：命令名（第一个参数）
    std::string cmd = g_argv[0];  

    //6.2：执行cd命令（切换目录）
    if (cmd == "cd")
    {
        Cd();    
        return true;
    }

    //6.3：执行echo命令（打印内容）
    else if (cmd == "echo")
    {
        Echo();  
        return true;
    }

    //6.4：执行export命令（将变量导出为环境变量）
    else if (cmd == "export")
    {
        Export();
        return true;
    }

    //6.5：实现alias命令（设置命令别名，如alias ll='ls -l'）
    else if (cmd == "alias")
    {
        Alias();
        return true;
    }

    //6.6：不是内置命令
    return false;   
}



//7.执行外部命令（创建子进程并进行进程替换）
int Execute()
{
    //7.1：创建子进程
    pid_t id = fork();

    //7.2：-------------------------- 子进程执行区域 --------------------------
    if (id == 0)
    {
        //第一步：定义文件描述符变量，用于重定向操作
        int fd = -1;  

        //第二步：根据全局重定向类型（redir）处理输入输出
        //情况一： 输入重定向（<）：从文件读取输入，替代标准输入（键盘）
        if (redir == INPUT_REDIR)
        {
            //（1）以只读模式打开目标文件（若文件不存在，open会失败）
            fd = open(filename.c_str(), O_RDONLY);

            //（2）打开失败则退出子进程（退出码1）
            if (fd < 0) exit(1);  

            //（3）将标准输入（fd=0）指向打开的文件
            dup2(fd, 0);         

            //（4）关闭文件描述符（已重定向，无需保留）
            close(fd);           
        }

        //情况二：输出重定向（>）：将输出写入文件，覆盖原有内容
        else if (redir == OUTPUT_REDIR)
        {
            //（1）以只写模式打开目标文件（不存在则创建，存在则截断）
            fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
            //注意：0666：文件权限（所有者/组/其他用户均有读写权限）
            
            //（2） 打开失败则退出子进程（退出码2）
            if (fd < 0) exit(2);  
            
            //（3）将标准输出（fd=1）指向打开的文件
            dup2(fd, 1);         
           
            //（4）关闭文件描述符fd
            close(fd);
        }

        //情况三：追加重定向（>>）：将输出追加到文件末尾
        else if (redir == APPEND_REDIR)
        {
            //（1）以只写模式打开目标文件（不存在则创建，存在则追加）
            fd = open(filename.c_str(), O_WRONLY| O_CREAT | O_APPEND, 0666);

            //（2）打开失败则退出子进程（退出码2）
            if (fd < 0) exit(2);  

            //（3）将标准输出（fd=1）指向打开的文件
            dup2(fd, 1);         

            //（4）关闭文件描述符fd
            close(fd);
        }

        //第三步：进行进程替换用外部命令替换子进程的代码和数据
        execvp(g_argv[0], g_argv); //注意：参数1：命令名（如"ls"）；参数2：参数数组（如["ls", "-l", NULL]）

        //第四步：退出子进程，退出码1表示"命令执行失败"
        exit(1);
    }

    //7.3：-------------------------- 父进程执行区域 --------------------------
    //第一步：存储子进程退出状态
    int status = 0;  

    //第二步：等待子进程退出（阻塞等待）
    pid_t rid = waitpid(id, &status, 0);
    if (rid > 0)
    {
        //注意：提取子进程的退出码（WEXITSTATUS宏用于从status中获取退出码）
        lastcode = WEXITSTATUS(status);
    }
    return 0;
}


//7.Shell主函数：实现Shell的核心循环
int main()
{
    //7.1：初始化环境变量（从父进程继承并添加自定义变量）
    InitEnv();

    //7.2：Shell主循环：持续接收并处理命令
    while (true)
    {
        //1.输出命令行提示符（如：[user@host dir]#）
        PrintCommandPrompt();

        //2.获取用户输入的命令行
        char commandline[COMMAND_SIZE];
        if (!GetCommandLine(commandline, sizeof(commandline)))
        {
            continue;  // 获取失败（如空命令）则重新循环
        }        

        //3.重定向分析（处理<、>、>>）
        RedirCheck(commandline);

        //4.命令行解析（将命令拆分为参数数组）
        if (!CommandParse(commandline)) 
        {
            continue;  // 解析失败则重新循环
            //注意细节：这里添加了continue，因为如果解析失败的话，是不能执行内置命令或者是外部命令的
        }

        //5.检测别名（实现别名替换逻辑）← 必须先进行别名替换
        CheckAndReplaceAlias();

        //6.执行内置命令
        if (CheckAndExecBuiltin())
        {
            continue;  //注意细节：这里添加了continue，因为执行了内置命令则跳过外部命令执行
        }

        //7.执行外部命令（创建子进程并替换）
        Execute();
    }
    return 0;
}


