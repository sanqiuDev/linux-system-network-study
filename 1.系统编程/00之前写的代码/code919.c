#include <stdio.h>
#include <string.h>

int main()
{
    //1.定义整型变量i并初始化为0
    int i = 0;

    //2.循环200次，遍历错误码0到199
    for (; i < 200; i++)
    {
        // 打印“错误码”及其对应的“错误信息字符串”
        printf("%d->%s\n", i, strerror(i)); //注意：strerror函数根据错误码返回对应的错误描述
    }

    //3.主函数正常返回0，表示程序成功执行结束
    return 0;
}
