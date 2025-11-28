//根据核心转储文件进行调试

#include <stdio.h>  
#include <signal.h>

int main()
{
    printf("hello linux\n");
    printf("hello linux\n");
    printf("hello linux\n");
    printf("hello linux\n");

    int a = 10;
    a /= 0;

    printf("hello linux\n");
    printf("hello linux\n");
    printf("hello linux\n");
}
