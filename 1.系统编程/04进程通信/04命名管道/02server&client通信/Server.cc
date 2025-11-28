#include "Common.hpp" 

// 主函数：作为命名管道的读端进程，负责创建管道文件并从管道读取数据
int main()
{
    //1.创建NamedFifo对象，用于创建命名管道文件
    NamedFifo fifo(PATH, FILENAME);

    //2.创建FileOperator对象，用于操作管道的读端
    FileOperator readerfile(PATH, FILENAME);

    //3.以只读方式打开命名管道
    readerfile.OpenForRead(); //注意：若此时写端未打开，该函数会阻塞，直到写端进程打开管道后才返回

    //4.持续从管道中读取数据并打印
    readerfile.Read(); //注意：当写端关闭时，read会返回0，循环会退出

    //5.关闭管道的读端文件描述符释放资源
    readerfile.Close();

    return 0;  
}