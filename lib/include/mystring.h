#pragma once

#include <stddef.h> //size_t 是无符号整数类型，它在 <stddef.h> 头文件中定义


//1.声明“自定义求文件的长度的函数”---> 模拟strlen
int my_strlen(const char* s);

//2.声明“自定义字符串比较函数”---> 模拟strcmp
int my_strcmp(const char* s1, const char* s2);

//3.声明“自定义内存拷贝函数”---> 模拟memcpy
void* my_memcpy(void* dest, const void* src, size_t n);

