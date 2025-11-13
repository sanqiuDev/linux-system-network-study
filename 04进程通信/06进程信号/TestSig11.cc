//使用volatile

#include <iostream>   
#include <unistd.h>  
#include <signal.h>  

//定义全局变量flag：用于标记是否收到退出信号
/* volatile关键字修饰：
*     1. 告知编译器该变量可能被意外修改（如信号处理函数）
*     2. 禁止编译器优化（避免将变量缓存到寄存器）
*/
volatile int flag = 0;

/*------------------------------------信号处理函数------------------------------------*/
void handler(int signu)
{
    //1.输出信号处理动作 ---> 当前flag的值将被改为1
    std::cout << "更改全局变量, " << flag << "-> 1" << std::endl;

    //2.将全局变量flag设为1 ---> 用于通知主循环退出
    flag = 1;
}

/*------------------------------------主函数------------------------------------*/
int main()
{
    //1.注册信号处理函数
    signal(2, handler); //当进程收到SIGINT信号时，会暂停主程序执行，转而执行handler

    //2.主循环：持续检查flag的值，当flag为1时退出循环
    while(!flag); //由于flag被volatile修饰，每次判断都会从内存中读取最新值（而非寄存器缓存）

    //3.循环退出后，输出正常退出信息
    std::cout << "process quit normal!" << std::endl;
    return 0;
}