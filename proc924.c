#include <stdio.h>
#include <string.h>

int main()
{
    //1.以只写模式打开名为"myfile"的文件，返回文件指针fp
    FILE* fp = fopen("myfile", "w");

    //2.检查文件是否成功打开，若失败则打印错误信息
    if (!fp)
    {
        printf("fopen error!\n");
        return 1;
    }

    //3.定义要写入文件的字符串内容
    const char* msg = "hello linux";

    //4.循环5次，向文件中写入内容
    int count = 1;
    while (count <= 5)
    {
        //4.1：定义缓冲区，用于存储每次要写入的内容
        char buffer[1024];
        //4.2：将msg和cnt格式化到buffer中，格式为"hello linux: 数字\n"，cnt自增
        snprintf(buffer, sizeof(buffer), "%s：%d\n", msg, count++);

        //4.3：将buffer指向的字符串写入到文件fp中，每次写入的长度为buffer的实际长度
        fwrite(buffer, strlen(buffer), 1, fp);
        //注意：fwrite主要作用是将内存中的数据块按指定大小和数量写入到指定文件中，适用于二进制文件和文本文件的写入操作
        /* fwrite的参数说明：
        *   第一个参数（const void* ptr）：指向要写入数据的内存起始地址（源数据地址）
        *     此处为buffer，即待写入的字符串首地址
        *
        *   第二个参数（size_t size）：每个数据块的大小（单位：字节）
        *     此处为strlen(buffer)，表示每个数据块是整个字符串的长度（不含结束符'\0'）
        *
        *   第三个参数（size_t nmemb）：要写入的数据块数量
        *     此处为1，表示写入1个大小为strlen(buffer)字节的数据块
        *
        *   第四个参数（FILE* stream）：目标文件指针，指向已打开的文件
        *     此处为fp，即之前用fopen打开的文件
        *
        *   返回值：
        *     1. 若写入成功，返回成功写入的数据块数量（正常情况下等于nmemb参数值）
        *     2. 若写入失败，返回值小于nmemb（如：磁盘空间不足、文件被关闭等）
        */
    }

    //5.关闭文件流，释放资源
    fclose(fp);

    //6.关闭后 fp 指针变为野指针,将其置空
    fp = NULL;

    printf("文件已经成功关闭\n");
    return 0;
}
