#pragma once

/*---------------------------------------头文件---------------------------------------*/
#include <iostream>    // 包含“标准输入输出流库” ---> 提供cout、endl等控制台输入输出功能
#include <cstdio>      // 包含“C标准输入输出库” ----> 提供printf等格式化输入输出函数
#include <string>      // 包含“字符串类库” --------> 提供std::string字符串类型及相关操作
#include <sys/types.h> // 包含“系统类型定义库” -----> 提供pid_t等系统相关数据类型定义
#include <sys/ipc.h>   // 包含“System V IPC键值相关库” ---> 提供key_t类型定义
#include <sys/shm.h>   // 包含“共享内存相关库” -----> 提供shmget、shmat、shmdt、shmctl等共享内存系统调用声明
#include <unistd.h>    // 包含“POSIX系统调用库” ----> 提供ftok函数及其他系统调用声明

#include "Common.hpp"  // 自定义公共头文件（推测包含ERR_EXIT错误处理宏）

/*---------------------------------------常量&宏---------------------------------------*/
//1.共享内存“默认标识符”（初始化为-1，表示未 创建/获取 共享内存）
//2.共享内存“默认大小”（4096字节，对应系统一页内存大小，避免内存对齐问题）
//3.共享内存“访问权限”
const int gshmid = -1;
const int gsize = 4096;
const int gmode = 0666;

//4.ftok函数使用的“文件路径”（当前目录）---> 用于生成唯一IPC键值
//5.ftok函数使用的“项目标识符”（0x66，仅低8位有效）---> 用于区分同一文件对应的不同IPC资源
const std::string PathName = ".";
const int ProjId = 0x66;


//1.宏定义：共享内存创建者身份标识
#define CREATER "creater"
//2.宏定义：共享内存使用者身份标识
#define USER "user"

/*---------------------------------------共享内存管理类---------------------------------------*/
class Shm
{
private:
    /*------------------------------私有属性------------------------------*/
    //1.共享内存标识符
    //2.IPC键值
    //3.共享内存大小
    //4.共享内存映射到当前进程的虚拟地址起始指针
    //5.用户身份（CREATER：创建者 / USER：使用者）

    int _shmid;            
    key_t _key;            
    int _size;             
    void *_start_mem;      
    std::string _user_type; 


    /*------------------------------私有方法------------------------------*/
    //1.“创建/获取 共享内存”
    void CreateHelper(int flg) //参数flg：shmget函数的标志位（如：IPC_CREAT、IPC_EXCL等）
    {
        //1.打印生成十六进制格式的IPC键值
        printf("key: 0x%x\n", _key);

        //2.调用shmget创建或获取共享内存
        _shmid = shmget(_key, _size, flg); 
        if (_shmid < 0)   // 创建/获取 失败
        {
            ERR_EXIT("shmget");  
        }
        printf("shmid: %d\n", _shmid); // 打印成功 创建/获取 的共享内存标识符
    }

    //2.“创建全新的共享内存”（仅创建者调用）
    void Create()
    {
        CreateHelper(IPC_CREAT | IPC_EXCL | gmode); 

        /* 注意事项：
        *     1. 共享内存就像是普通文件一样也有所谓的权限
        *     2. 创建共享内存的时候是必需要指定权限的
        *     3. 如果不指定权限的话，无法将共享内存映射到当前进程的虚拟地址空间
        */
    }

    //3.“将共享内存映射到当前进程的虚拟地址空间”
    void Attach()
    {
        
        /*shmat的参数说明：
        *  参数1：共享内存标识符shmid
        *  参数2：nullptr：让系统自动分配虚拟地址（推荐用法）
        *  参数3：0：默认映射权限（读写权限，与共享内存创建时的权限一致）
        */

        //1.调用shmat将共享内存映射到当前进程的虚拟地址空间
        _start_mem = shmat(_shmid, nullptr, 0);
        if ((long long)_start_mem < 0)  // 映射失败判断：shmat失败返回(void*)-1，需强转为long long避免指针判断误差
        {
            ERR_EXIT("shmat"); 
        }
        /*注意：上面我们将_start_mem的值强转为了long long的类型是因为：
        *   1. 当前我们使用的系统是64位的系统
        *   2. 在64位的系统中一个指针占用8字节的空间
        *   3. 一个long long类型的正好是占用8字节的空间
        *   4. 如果强转为int类型（占4字节）去和0进行对比的话，会有精度损失
        */
        printf("attach success\n");  // 映射成功提示
    }

    //4.“解除共享内存与当前进程虚拟地址空间的映射”
    void Detach()
    {
        //1.调用shmdt解绑
        int n = shmdt(_start_mem);
        if (n == 0)  // 解绑成功（返回0）
        {
            printf("detach success\n");  
        }
    }

    //5.“获取已存在的共享内存”（仅使用者调用）
    void Get()
    {
        //1.调用辅助函数，标志位仅用IPC_CREAT：
        CreateHelper(IPC_CREAT);  

        /* 解释：为什么要将CreateHelper封装起来？
        *  回答：Create方法和Get方法的实现几乎一模一样的，唯一的区别就是
        *     1. Create调用shmget的时候传入的第3参数是：IPC_CREAT | IPC_EXCL | gmode
        *     2. Get调用shmget的时候传入的第3参数是：IPC_CREAT
        */
    }

    //6.“销毁共享内存”（仅创建者调用）
    void Destroy()
    {
        //1.先解除当前进程与共享内存的映射
        Detach();

        //2.仅创建者有权限删除共享内存
        if (_user_type == CREATER)
        {
            //1.调用shmctl删除共享内存
            int n = shmctl(_shmid, IPC_RMID, nullptr);
            if (n == 0)  // 删除成功
            {
                printf("shmctl delete shm: %d success!\n", _shmid);  
            }

            else  // 删除失败
            {
                ERR_EXIT("shmctl"); 
            }
        }
    }

public:
    //1.“构造函数” ---> 初始化共享内存相关参数，根据用户身份 创建/获取 共享内存并完成映
    Shm(const std::string &pathname, int projid, const std::string &usertype) //这里需要注意的是：usertype需要进行参数传递
        : _shmid(gshmid),      // 初始化共享内存标识符为默认值（未创建）
          _size(gsize),        // 初始化共享内存大小为默认值（4096字节）

          _start_mem(nullptr), // 初始化虚拟地址起始指针为nullptr（未映射）

          _user_type(usertype)  // 初始化用户身份
    {
        //1.调用ftok生成唯一的IPC键值（用于关联共享内存）
        _key = ftok(pathname.c_str(), projid);
        if (_key < 0) 
        {
            ERR_EXIT("ftok");  
        }

        //2.根据用户身份执行不同操作
        //情况一：身份为创建者 ---> 创建全新共享内存
        if (_user_type == CREATER)
        {
            Create();  
        }
        //情况二：身份为使用者 ---> 获取已存在的共享内存
        else if (_user_type == USER)
        {
            Get(); 
        }
        //情况三：身份无效：无操作（可根据需求添加错误处理）
        else
        { }

        //3.无论创建还是获取，最终都将共享内存映射到当前进程虚拟地址空间
        Attach();
    }

    //2.“析构函数” ---> 清理共享内存资源（仅创建者执行销毁操作）
    ~Shm()
    {
        //1.打印当前用户身份
        std::cout << _user_type << std::endl;

        //2.仅创建者需要销毁共享内存（使用者仅需解绑，无需删除）
        if(_user_type == CREATER)
        {
            Destroy();
        }
    }

    //3.“获取共享内存映射到当前进程的虚拟地址起始指针”
    void *GetPtr()
    {
        //1.以十六进制格式打印虚拟地址
        printf("VirtualAddr: %p\n", _start_mem);

        //2.返回虚拟地址指针（供进程读写共享内存）
        return _start_mem;  
    }

    //4.“获取共享内存的大小”
    int GetSize()
    {
        return _size;  
    }

    /* 根据需要的进行：注释or打开
    //5.“查看共享内存的属性信息”
    void GetStat()
    {
        /*shmctl的参数的说明：
        *   参数1：shmid：共享内存标识符
        *   参数2：IPC_STAT：获取属性的命令
        *   参数3：&ds：输出型参数，存储获取到的属性信息
        */
    /*
        //1.定义共享内存属性结构体（存储共享内存的详细信息）
        struct shmid_ds ds;  

        //2.调用shmctl获取共享内存属性
        int n = shmctl(_shmid, IPC_STAT, &ds);
        if (n < 0) 
        {
            ERR_EXIT("shmctl IPC_STAT");
        }

        //3.打印共享内存大小、对应的IPC键值
        printf("shm_segsz: %ld\n", ds.shm_segsz);  
        printf("key: 0x%x\n", ds.shm_perm.__key); 
    }
    */
};