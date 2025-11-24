#include <stdio.h>      
#include <unistd.h>     
#include <fcntl.h>      // 提供 open 函数的标志常量（O_CREAT 等）
#include <string.h>   

int main()
{
    //1.关闭标准错误流（文件描述符 2 通常对应标准错误输出，默认输出到显示器）
    close(2);

    //2.打开 log.txt 文件
    //注意：由于之前关闭了文件描述符 2，此处 open 返回的文件描述符 fd 会复用 2
    int fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1)  
    {
        perror("open failed"); 
        return 1;
    }

    //3.使用perror输出内容（perror默认写入标准错误流stderr，即文件描述符 2）
    //注意：由于fd复用了2号描述符，此时所有stderr输出会被重定向到log.txt
    perror("fd: %d");        // 实际输出格式："fd: %d: Success"（因当前无错误，默认Success）
    perror("hello printf");  // 输出到log.txt，格式："hello printf: Success"
    perror("hello printf");  // 同上，stderr无缓冲，直接写入内核缓冲区
    perror("hello printf");  // 无需fflush，因stderr默认无用户级缓冲区

    //4.定义要写入的字符串
    const char* msg = "hello write";  // 
    //5.向文件描述符 fd 对应的文件写入数据，长度为字符串实际长度（不含结束符'\0'）
    //注意：write是系统调用，无用户级缓冲区，直接将数据写入内核缓冲区
    write(fd, msg, strlen(msg));

    //6.关闭文件描述符 fd，释放资源
    close(fd);

    return 0;
}

