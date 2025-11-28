//查看codedump标志位

#include <stdio.h>  
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    //1.创
    pid_t id = fork();

    //2.子
    if (id == 0)
    {
        sleep(2);
        printf("hello linux\n");
        printf("hello linux\n");
        printf("hello linux\n");


        int a = 10;
        a /= 0;

        printf("hello linux\n");
        printf("hello linux\n");
        printf("hello linux\n");
    }

    //3.父
    int status = 0;
    waitpid(id, &status, 0);
    printf("signal: %d, exit code: %d, core dump: %d\n",
           (status & 0x7F), (status >> 8) & 0xFF, (status >> 7) & 0x1);
}