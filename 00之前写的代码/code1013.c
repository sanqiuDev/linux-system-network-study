#include <stdio.h>    

//1.声明外部变量environ，
extern char** environ; //它是一个指向字符指针数组的指针，用于存储环境变量

int main(int argc, char* argv[])
{
    //1.(void)变量名：告诉编译器该变量已声明但暂不使用，避免"未使用变量"的警告
    (void)argc;  
    (void)argv;  

    //2.遍历environ数组，打印所有环境变量
    for (int i = 0; environ[i]; i++) //注意：environ数组以NULL指针结尾，因此循环条件为environ[i] != NULL
    {
        // 打印格式：environ[索引] -> 环境变量字符串（格式为"变量名=变量值"）
        printf("environ[%d] -> %s\n", i, environ[i]);
    }

    //3.程序正常退出，返回0（约定俗成的成功退出码）
    return 0;
}

