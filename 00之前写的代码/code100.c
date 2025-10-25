#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
    //1.检查命令行参数数量是否为2（程序名本身算一个参数，所以期望还有一个选项参数）
    if (argc != 2)
    {
        //1.1：如果参数数量不对会打印用法提示
        printf("Usage: %s [-a|-b|-c]\n", argv[0]);
        
        //1.2
        return 1;
    }

    //2.获取命令行传入的第二个参数（选项参数）
    const char* arg = argv[1];

    //3.比较参数，判断是哪个功能选项
    if (strcmp(arg, "-a") == 0)
        printf("这是功能1\n");
    else if (strcmp(arg, "-b") == 0)
        printf("这是功能2\n");
    else if (strcmp(arg, "-c") == 0)
        printf("这是功能3\n");

    //4.如果参数不是 -a、-b、-c 中的任何一个，打印用法提示
    else
        printf("Usage: %s [-a|-b|-c]\n", argv[0]);

    return 0;
}

