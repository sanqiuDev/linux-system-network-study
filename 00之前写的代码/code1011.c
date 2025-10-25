#include <stdio.h>  // 包含“标准输入输出”头文件 ---> 用于 printf 等函数
#include <string.h> // 包含“字符串操作”头文件 -----> 用于 strcmp 等函数
#include <stdlib.h> // 包含“标准库”头文件 ---------> 用于 exit 等函数
#include <dirent.h> // 包含“目录操作”头文件 -------> 用于 DIR、struct dirent 等类型以及 opendir、readdir、closedir 函数
#include <sys/types.h> // 包含“系统类型”头文件 ----> 提供一些系统相关的类型定义
#include <unistd.h>    // 包含 Unix 标准头文件 ------> 用于一些 Unix 系统调用等


//主函数：接收命令行参数，遍历指定目录并打印文件名及其inode号
int main(int argc, char* argv[])
{
    //1.检查命令行参数数量是否为 2，若不是则输出用法提示并退出程序
    if (argc != 2)
    {
        //1.1：向标准错误流输出用法提示
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);

        //1.2：以失败状态退出程序
        exit(EXIT_FAILURE);  
    }

    //2.打开指定目录，获取目录流指针
    //2.1：opendir函数接收目录路径，返回DIR*类型的目录流指针（类似文件流）
    DIR* dir = opendir(argv[1]);
    //opendir的功能：打开指定目录，返回目录流指针（供后续操作），失败返回 NULL
    /* 函数的原型：DIR *opendir(const char *name);
    *  参数介绍：
    *       1. const char *name：指向目录路径字符串的指针，支持绝对路径和相对路径
    *       2. 参数为const类型，表明函数不会修改传入的路径字符串
    *
    *  返回值：
    *       1. 成功时，返回非空的DIR*类型目录流指针，用于后续readdir、closedir等操作
    *       2. 失败时，返回NULL，并设置errno变量（如：ENOENT表示目录不存在，EACCES表示权限不足）
    */


    //2.2：检查目录是否成功打开（若dir为NULL则表示打开失败）
    if (!dir)
    {
        perror("opendir");
        exit(EXIT_FAILURE); 
    }

    //3.定义目录项结构体指针，用于存储读取到的目录项信息
    struct dirent* entry; //注意：struct dirent是目录项结构体，包含文件名、inode号等信息

    //4.循环读取目录中的所有目录项
    //readdir的功能：从目录流中读取下一个目录项，返回包含目录项信息的结构体指针，读取到末尾或失败返回NULL
    /* 函数的原型：struct dirent *readdir(DIR *dirp);
    *  参数介绍：
    *       1. DIR *dirp：指向已打开的目录流指针（由opendir返回）
    *       2. 函数会自动推进目录流的内部指针，每次调用获取下一个目录项
    *
    *  返回值：
    *       1. 成功时，返回指向struct dirent结构体的指针，包含文件名、inode号等信息
    *       2. 失败或读取到目录末尾时，返回NULL，若为错误会设置errno变量
    */
    while ((entry = readdir(dir)) != NULL) 
    { 
        //4.1：跳过特殊目录项：当前目录（"."）和上级目录（".."）
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) //注意：strcmp比较文件名，等于0表示字符串相同
        {
            continue;  // 跳过本次循环，继续读取下一个目录项
        }

        // 4.2 打印文件名和对应的inode号
        /* 注意：
        *   entry->d_name：存储文件名的字符串
        *   entry->d_ino：存储文件的inode号（unsigned long类型）
        */
        printf("Filename: %s, Inode: %lu\n", entry->d_name, (unsigned long)entry->d_ino);
    }

    //5.关闭目录流，释放资源（类似关闭文件）
    closedir(dir);
    //closedir的功能：关闭已打开的目录流，释放相关资源，成功返回0，失败返回-1
    /* 函数的原型：int closedir(DIR *dirp);
    *  参数介绍：
    *       1. DIR *dirp：指向已打开的目录流指针（由opendir返回的有效指针）
    *       2. 若传入无效指针（如：NULL或已关闭的目录流），可能导致未定义行为
    *       3. 调用后该目录流指针将失效，不可再用于readdir等操作
    *
    *  返回值：
    *       1. 成功时，返回0，表示目录流已成功关闭并释放资源
    *       2. 失败时，返回-1，并设置errno变量（如：EBADF表示目录流无效）
    */

    return 0; 
}

