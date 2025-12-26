//对handler表的理解
#include <iostream>    
#include <unistd.h>    
#include <signal.h> 

void handler(int sig)
{
    std::cout << "hello sig: " << sig << std::endl;

    //第三版：恢复信号的默认处理行为
    signal(2, SIG_DFL); 
    std::cout << "恢复处理动作" << std::endl;
}

int main()
{
    //自定义捕捉信号
    signal(2, handler); 

    // //第二版：忽略信号
    // signal(2, SIG_IGN); 
 
    while(true)
    {
        sleep(1);
        std::cout << "." << std::endl;
    }
    return 0;
}
