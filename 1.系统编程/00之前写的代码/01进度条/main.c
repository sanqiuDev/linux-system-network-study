#include "process.h"

/*--------------------------设置初始状态的信息--------------------------*/
//1.定义总数据量（注：单位可自行假设，这里为 5.0M）
double total = 5.0;

//2.定义每次下载的数据量（注：单位可自行假设，这里为 1.0M）
double speed = 1.0;

/*--------------------------模拟下载过程的函数--------------------------*/
void Download()
{
    //1.记录当前已下载的数据量
    double current = 0;

    //2.当已下载数据量小于总数据量时继续循环
    while (current < total)
    {
        //2.1：调用进度条函数
        process_v1();            //调用 version1 版本的函数
        //process_v2(total, current); //调用 version2 版本的函数

        //2.2：模拟下载数据的耗时休眠 3000 微秒
        usleep(3000);

        //2.3：已下载数据量增加 speed
        current += speed;
    }
    //针对于 version2 版本：循环结束后，current 已经等于 total，额外调用一次进度条函数，确保 100% 的进度被显示出来
    process_v2(total, current); //注意：这是个细节注意一下

    //3.下载完成后，输出提示信息，显示最终的已下载数据量
    printf("\ndownload %.2lfMB Done\n", current);
}

/*--------------------------程序入口的主函数--------------------------*/
int main()
{
    Download();
    return 0;
}