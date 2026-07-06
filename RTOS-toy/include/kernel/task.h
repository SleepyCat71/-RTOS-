//
// Created by 81432 on 2026/7/4.
//

#ifndef RTOS_TOY_TASK_H //检查RTOS_TOY_TASK_H是否已经被声明
#define RTOS_TOY_TASK_H //若未声明则在此声明

#include <cstdint> //定义了uint8_t与uint32_t
#include <string> //包含std::string，方便存储任务名
#include <functional> //多态函数包装器，可以保存任何可调用对象

enum /*强类型枚举，不会隐式泄露*/class TaskState : uint8_t //uint8_t指定底层存储类型为1字节
{
    READY = 0,//任务就绪，等待选中
    RUNNING = 1,//正在CPU中执行
    BLOCKED = 2,//任务被阻塞，暂停状态
    TERMINATED = 3//任务执行完毕
};

struct TaskControlBlock//定义新的类，并不进行封装所以用struct
{
    std::string name;//任务名称，移植时可以换成定长char name[16]
    std::function<void()> entry;//任务入口函数，签名是void意味着不接受参数和返回值
    uint8_t priority;//任务优先级，数值越高优先级越高
    TaskState state;//当前状态，READY/RUNNING/BLOCK/TERMINATED，调度器将会选出READY状态的任务
    uint32_t total_ticks_run;//累计运行时间（ticks），是性能统计信息
    uint32_t context_switch_count;//上下文切换次数，任务每次被选中都会加1。可以反映任务被切换进出的频繁程度
    void*/*不知道具体什么IPC对象所以用最通用的指针类型*/ blocked_on;//指向阻塞原因对象的裸指针，若状态是BLOCKED，则指向任务等待的IPC对象

    TaskControlBlock(const std::string& name,//用const传递避免拷贝
                    std::function<void()> entry,
                    uint8_t priority);
};//只声明了构造函数，在.cpp文件中完成，它接受任务名、入口函数和优先级三个参数

#endif

