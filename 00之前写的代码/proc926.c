#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>  // 用于bool类型和true/false宏定义

int main()
{
    //1.设置文件创建掩码为0
    umask(0);

    //2.以只读方式打开log.txt文件，O_RDONLY表示只读模式   
    int fd = open("log.txt", O_RDONLY); //注意：若文件不存在，open会失败并返回-1

    //3.判断文件是否打开失败，若fd小于0则打开失败
    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    //4.打印成功打开文件后得到的文件描述符fd的值（通常为3，0/1/2被标准输入/输出/错误占用）
    printf("fd: %d\n", fd);

    //5.无限循环，用于持续读取文件内容直到文件末尾
    while (true)
    {
        //5.1：定义一个大小为64的字符数组，用于存储读取到的数据
        char buffer[64];

        //5.2：调用read系统调用从文件读取数据
        int n = read(fd, buffer, sizeof(buffer) - 1);
        /* read的参数说明：
        *      1. 参数1：文件描述符fd（指定要读取的文件）
        *      2. 参数2：缓冲区地址buffer（存储读取到的数据）
        *      3. 参数3：读取的最大字节数（sizeof(buffer)-1，预留1字节给字符串结束符）
        *  返回值n：实际读取到的字节数（0表示文件末尾，-1表示出错）
        */

        //5.3：分支1：读取到有效数据（n > 0）
        if (n > 0)
        {
            //第一步：在读取到的数据末尾添加字符串结束符'\0'，确保printf能正常解析字符串
            buffer[n] = '\0';

            //第二步：打印读取到的字符串（不换行，保持原文件格式）
            printf("%s", buffer);
        }
        //5.4：分支2：读取到文件末尾（n == 0）
        else if (n == 0)
        {
            //第一步：打印换行，使输出更整洁
            printf("\n");

            //第二步：跳出循环，结束读取
            break;
        }
        //5.5：分支3：读取出错（n < 0）
        else
        {
            //第一步：打印read函数的错误信息（如中断、权限问题等）
            perror("read");

            //第二步：跳出循环，结束读取
            break;
        }
    }

    //6.关闭文件描述符，释放系统资源
    close(fd);
    return 0;  
}

