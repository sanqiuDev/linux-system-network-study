#include <stdio.h>    
#include <stdlib.h>   // 提供malloc等内存分配函数的声明

//1.未初始化的全局变量，会被存放在“BSS段”
int g_unval;

//2.已初始化的全局变量，会被存放在“数据段”
int g_val = 100;

int main(int argc, char* argv[], char* env[])
{
    //3.定义一个字符串常量，存储在“只读数据段”
    const char* str = "helloworld";

    //4.定义静态局部变量，存储在“数据段”（与全局变量同区域）
    static int test = 10;

    //5.在堆上分配内存，返回的地址属于堆区域
    char* heap_mem = (char*)malloc(10);
    char* heap_mem1 = (char*)malloc(10);
    char* heap_mem2 = (char*)malloc(10);
    char* heap_mem3 = (char*)malloc(10);

    //6.打印main函数代码的地址（代码段）
    printf("code addr: %p\n", main);
    //7.打印已初始化全局变量g_val的地址（数据段）
    printf("init global addr: %p\n", &g_val);
    //8.打印未初始化全局变量g_unval的地址（BSS段）
    printf("uninit global addr: %p\n", &g_unval);

    //9.打印堆上分配的内存地址（堆区域）
    printf("heap addr: %p\n", heap_mem);
    printf("heap addr: %p\n", heap_mem1);
    printf("heap addr: %p\n", heap_mem2);
    printf("heap addr: %p\n", heap_mem3);

    //10.打印静态局部变量test的地址（数据段）
    printf("test static addr: %p\n", &test);

    //11.打印栈上变量heap_mem的地址（栈区域）
    printf("stack addr: %p\n", &heap_mem);
    printf("stack addr: %p\n", &heap_mem1);
    printf("stack addr: %p\n", &heap_mem2);
    printf("stack addr: %p\n", &heap_mem3);

    //12.打印字符串常量str的地址（只读数据段）
    printf("read only string addr: %p\n", str);

    //13.遍历命令行参数数组，打印每个参数的地址
    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d]: %p\n", i, argv[i]);
    }

    //14.遍历环境变量数组，打印每个环境变量的地址（环境变量通常在栈或特定区域）
    for (int i = 0; env[i]; i++)
    {
        printf("env[%d]: %p\n", i, env[i]);
    }

    //15.程序正常退出，返回0
    return 0;
}
