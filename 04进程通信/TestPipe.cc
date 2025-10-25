#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>

void ChildWrite(int wfd)
{
    char buffer[1024];
    int cnt = 0;
    while (true)
    {
        snprintf(buffer, sizeof(buffer), "I am child, pid: %d, cnt: %d", getpid(), cnt++);
        write(wfd, buffer, strlen(buffer));
        sleep(1);
    }
}

void FatherRead(int rfd)
{
    char buffer[1024];
    while (true)
    {
        buffer[0] = 0;
        ssize_t n = read(rfd, buffer, sizeof(buffer)-1);
        if(n > 0)
        {
            buffer[n] = 0;
            std::cout << "child say: " << buffer << std::endl;
        }
    }
}

int main()
{
    // 1. 创建管道
    int fds[2] = {0}; // fds[0]:读端   fds[1]: 写端
    int n = pipe(fds);
    if (n < 0)
    {
        std::cerr << "pipe error" << std::endl;
        return 1;
    }
    std::cout << "fds[0]: " << fds[0] << std::endl;
    std::cout << "fds[1]: " << fds[1] << std::endl;

    // 2. 创建子进程
    pid_t id = fork();
    if (id == 0)
    {
        // child
        // code
        // 3. 关闭不需要的读写端，形成通信信道
        // f -> r, c -> w
        close(fds[0]);
        ChildWrite(fds[1]);
        close(fds[1]);
        exit(0);
    }
    // 3. 关闭不需要的读写端，形成通信信道
    // f -> r, c -> w
    close(fds[1]);
    FatherRead(fds[0]);
    waitpid(id, nullptr, 0);
    close(fds[0]);

    return 0;
}