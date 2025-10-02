#include <stdio.h>      
#include <unistd.h>     
#include <fcntl.h>      // 提供 open 函数的标志常量（O_CREAT 等）
#include <string.h>   

int main() 
{
    //1.关闭标准输出（文件描述符 1 通常对应显示器输出）
    close(1); 

    //2.打开 log.txt 文件
    int fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
    //注意：因为之前关闭了文件描述符 1，所以这里 open 返回的文件描述符 fd 是 1
    if (fd == -1) 
    { 
        perror("open failed");
        return 1;
    }

    //3.库函数 printf 输出，由于标准输出被重定向到 log.txt（文件描述符 1 现在对应 log.txt），所以会写入该文件
    printf("fd: %d\n", fd);    // 打印文件描述符 fd 的值，这里 fd 应该为 1
    printf("hello printf\n");   // 向 log.txt 写入 "hello printf\n"
    printf("hello printf\n");   // 向 log.txt 写入 "hello printf\n"
    printf("hello printf\n");   // 向 log.txt 写入 "hello printf\n"
    //fflush(stdout); // 刷新标准输出缓冲区，确保库函数的输出立即写入文件

    //4.系统调用 write 输出，直接向 fd（即：log.txt）写入数据
    const char* msg = "hello write\n";  // 定义要写入的字符串

    //5.向文件描述符 fd 对应的文件写入 msg 指向的字符串，写入长度为 strlen(msg)（即字符串实际长度）
    write(fd, msg, strlen(msg));

    //6.关闭文件描述符 fd，释放资源，确保数据正确写入并关闭文件
    close(fd);

    return 0;
}
