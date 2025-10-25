//展示C/C++的标准输出和标准错误
#include <cstdio>
#include <iostream>

int main()
{
    /*------------------------ 向标准输出（stdout，对应文件描述符1）进行打印 ------------------------*/
    //1.C++中的cout是标准输出流对象，用于输出数据到控制台
    std::cout << "hello cout" << std::endl;
    //2.C语言的printf函数，向标准输出打印内容
    printf("hello printf\n");

    /*------------------------ 向标准错误（stderr，对应文件描述符2）进行打印 ------------------------*/
    //3.C++中的cerr是标准错误流对象，用于输出错误信息到控制台，默认无缓冲，直接输出
    std::cerr << "hello cerr" << std::endl;
    //4.C语言的fprintf函数，指定输出到stderr（标准错误流），打印内容并换行
    fprintf(stderr, "hello stderr\n");
    return 0;
}

