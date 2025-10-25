
#include <stdio.h>
#include <string.h>
#include <unistd.h>

//1.声明version1版本的进度条函数
//2.声明version2版本的进度条函数 --->  区别之处：接收“总数据量”和“当前已处理数据量参数”

void process_v1();
void process_v2(double total, double current);;
