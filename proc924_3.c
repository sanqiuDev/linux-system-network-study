#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
    //1.检查命令行参数个数是否为2（程序名+1个文件名参数）
    if (argc != 2)
    {
        printf("argv error!\n");  
        return 1; 
    }

    //2.以只读模式打开命令行参数指定的文件，返回文件指针fp
    FILE* fp = fopen(argv[1], "r");
    if (!fp)
    {
        printf("fopen error!\n");  
        return 2;  
    }

    //3.无限循环，直到遇到文件结束标志才退出
    while (1) 
    {
        //3.1：定义用于存储读取内容的缓冲区，大小为1024字节
        char buf[1024];

        //3.2：fread函数：从fp指向的文件中读取数据到buf
        int s = fread(buf, 1, sizeof(buf), fp);
        //注意：此处每次读取sizeof(buf)个1字节的数据块（即最多读1024字节）

        //3.3：如果读取到的字节数大于0，说明有有效数据读取
        if (s > 0)
        {
            //第一步：在读取内容的末尾添加字符串结束符'\0'，使其成为合法字符串
            buf[s] = 0;  

            //第二步：打印读取到的内容
            printf("%s", buf); 
        }

        //3.4：检查是否到达文件末尾：feof返回非0表示已到文件末尾
        if (feof(fp))
        {
            break;  // 跳出循环，结束读取
        }
    }

    //4.关闭文件流，释放文件资源
    fclose(fp); 
    fp = NULL;
    return 0;  
}
