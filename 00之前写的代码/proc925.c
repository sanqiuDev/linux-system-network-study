#include <stdio.h>
//1. 定义四个标志位宏，每个标志对应整数的一个独立二进制位
/*
*   标志位设计原理：利用二进制位的唯一性，每个标志占用一个独立的位，
*   通过位运算实现多状态的组合与判断，节省内存且操作高效
*
*   1. ONE_FLAG 对应第 0 位（二进制最低位），值为 1（二进制 0001）
*      计算方式：1 << 0 等价于 1（2的0次方）
*
*   2. TWO_FLAG 对应第 1 位，值为 2（二进制 0010）
*      计算方式：1 << 1 等价于 2（2的1次方）
*
*   3. THREE_FLAG 对应第 2 位，值为 4（二进制 0100）
*      计算方式：1 << 2 等价于 4（2的2次方）
*
*   4. FOUR_FLAG 对应第 3 位，值为 8（二进制 1000）
*      计算方式：1 << 3 等价于 8（2的3次方）
*/
#define ONE_FLAG   (1 << 0)  // 二进制：0001，十进制：1
#define TWO_FLAG   (1 << 1)  // 二进制：0010，十进制：2
#define THREE_FLAG (1 << 2)  // 二进制：0100，十进制：4
#define FOUR_FLAG  (1 << 3)  // 二进制：1000，十进制：8

//2. 打印函数：根据传入的标志位组合，输出对应的文本信息
//   参数 flags：多个标志位通过按位或（|）组合的整数
void Print(int flags)
{
    //2.1：判断 flags 是否包含 ONE_FLAG
    //原理：按位与（&）运算，若结果非0，则表示包含该标志
    //注意：ONE_FLAG 二进制为 0001，只有当 flags 的第0位为1时，结果才非0
    if (flags & ONE_FLAG)
    {
        printf("One!\n");  // 包含 ONE_FLAG 时输出
    }

    //2.2：判断 flags 是否包含 TWO_FLAG
    // 注意：TWO_FLAG 二进制为 0010，检查第1位是否为1
    if (flags & TWO_FLAG)
    {
        printf("Two\n");   // 包含 TWO_FLAG 时输出
    }

    //2.3：判断 flags 是否包含 THREE_FLAG
    //注意：THREE_FLAG 二进制为 0100，检查第2位是否为1
    if (flags & THREE_FLAG)
    {
        printf("Three\n"); // 包含 THREE_FLAG 时输出
    }

    //2.4：判断 flags 是否包含 FOUR_FLAG
    //注意：FOUR_FLAG 二进制为 1000，检查第3位是否为1
    if (flags & FOUR_FLAG)
    {
        printf("Four\n");  // 包含 FOUR_FLAG 时输出
    }
}

int main()
{
    //测试1：仅传入 ONE_FLAG
    //   此时 flags 二进制为 0001，Print函数只会触发 ONE_FLAG 的打印
    Print(ONE_FLAG);
    printf("\n");  // 打印空行分隔不同测试用例

    //测试2：传入 ONE_FLAG | TWO_FLAG（按位或组合）
    //   按位或运算后二进制为 0011（0001 | 0010），包含前两个标志
    Print(ONE_FLAG | TWO_FLAG);
    printf("\n");

    //测试3：传入三个标志组合（ONE_FLAG | TWO_FLAG | THREE_FLAG）
    //   二进制为 0111（0001 | 0010 | 0100），包含前三个标志
    Print(ONE_FLAG | TWO_FLAG | THREE_FLAG);
    printf("\n");

    //测试4：传入四个标志组合（全部标志）
    //   二进制为 1111（0001 | 0010 | 0100 | 1000），包含所有标志
    Print(ONE_FLAG | TWO_FLAG | THREE_FLAG | FOUR_FLAG);
    printf("\n");

    //测试5：传入不连续的标志组合（ONE_FLAG | FOUR_FLAG）
    //   二进制为 1001（0001 | 1000），包含第0位和第3位标志
    Print(ONE_FLAG | FOUR_FLAG);
    printf("\n");

    return 0; 
}
