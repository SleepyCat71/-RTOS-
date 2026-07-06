//
// Created by 81432 on 2026/7/4.
//

#ifndef RTOS_TOY_IPC_H
#define RTOS_TOY_IPC_H

#include <cstdint>
#include <queue>
#include <vector>
#include <string>
#include <functional>

struct TaskControlBlock;//前向声明，避免了包含task.h导致相互包含的循环依赖

enum class IPCType : uint8_t//标识三种IPC对象类型，uint8_t作为底层存储可以节省空间
{
    SEMAPHORE =0,
    MUTEX =1,
    QUEUE =2
};

struct Semaphore//信号量
{
    int32_t count;//信号量计数，count即可用资源数，原则上保证不出现负数。由于需要表达小于0的可能性来检测逻辑错误，故而使用int32_t
    std::vector<TaskControlBlock*> waiting_tasks;//动态数组，为避免存副本导致的数据不一致，元素为指向TCB的指针而非值。

    explicit Semaphore(int32_t initial_count);//构造函数，explicit防止隐式转换，属于安全措施
    bool wait(TaskControlBlock* task,uint32_t tick);//测试操作，任务请求获取信号量
    //若count>0，count--并返回true，若count==0，把task加入waiting_tasks，返回false
    //tick是当前时间戳，用于记录任务开始时间
    bool signal(TaskControlBlock* task);//增加操作，任务释放信号量：count++
    //若有等待任务，从waiting_tasks中取出一个，返回true表示有任务被唤醒
    //参数task用于执行signal的任务，
};

struct Mutex
{
    TaskControlBlock* owner;
    uint8_t original_priority;
    std::vector<TaskControlBlock*> waiting_tasks;

    Mutex();
    bool lock(TaskControlBlock* task,uint32_t tick);
    bool unlock(TaskControlBlock* task);
};

struct Message
{
    std::string data;
    uint32_t timestamp;
};

struct MessageQueue
{
    uint32_t max_size;
    std::queue<Message> messages;
    std::vector<TaskControlBlock*> waiting_send;
    std::vector<TaskControlBlock*> waiting_recv;

    explicit MessageQueue(uint32_t max_size);
    bool send(TaskControlBlock* task,const std::string& data, uint32_t tick);
    bool recv(TaskControlBlock* task,uint32_t tick,Message& out);
};

#endif
