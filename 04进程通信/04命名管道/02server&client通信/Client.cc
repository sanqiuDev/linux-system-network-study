#include "Common.hpp" 

// 主函数：作为命名管道的写端进程，负责向管道写入数据
int main()
{

    //注意：客户端不需要创建管道文件了，只需要打开服务端创建的管道文件就行了

    //1.创建FileOperator对象，指定管道文件的路径（PATH宏定义为当前目录）和名称（FILENAME宏定义为"fifo"）
    FileOperator writerfile(PATH, FILENAME); //注意：该对象用于管理管道的写端操作

    //2.以只写方式打开命名管道
    writerfile.OpenForWrite(); //注意：若此时读端未打开，该函数会阻塞，直到读端进程打开管道后才返回

    //3.持续读取用户输入，并将消息写入管道
    writerfile.Write();

    //4.关闭管道的写端文件描述符释放资源
    writerfile.Close();

    return 0;  
}

