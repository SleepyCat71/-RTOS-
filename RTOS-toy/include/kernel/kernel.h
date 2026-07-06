#ifndef RTOS_TOY_KERNEL_H
#define RTOS_TOY_KERNEL_H

#include <cstdint>
#include <vector>
#include "task.h"
#include "scheduler.h"

class Kernel {
public:
    static Kernel& instance();

    void init(Scheduler* sched);
    void create_task(TaskControlBlock* task);

    // Execute one system tick, returns false if all tasks done
    bool tick();

    void block_task(TaskControlBlock* task);
    void wakeup_task(TaskControlBlock* task);

    Scheduler* scheduler() const;
    uint32_t current_tick() const;
    TaskControlBlock* current_task() const;
    const std::vector<TaskControlBlock*>& all_tasks() const;

private:
    Kernel() = default;
    ~Kernel() = default;
    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;

    bool check_alive();

    std::vector<TaskControlBlock*> all_tasks_;
    Scheduler* scheduler_ = nullptr;
    TaskControlBlock* current_task_ = nullptr;
    uint32_t tick_ = 0;
};

#endif
