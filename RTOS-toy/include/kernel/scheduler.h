//
// Created by 81432 on 2026/7/5.
//

#ifndef RTOS_TOY_SCHEDULER_H
#define RTOS_TOY_SCHEDULER_H

#include <cstdint>
#include <vector>
#include <string>
#include "task.h"

enum class SchedAlgo : uint8_t
{
    FIFO = 0,
    PRIORITY = 1
};

class Scheduler
{
    public:
    virtual ~Scheduler() = default;
    virtual std::string name() const = 0;
    virtual SchedAlgo algo() const = 0;
    virtual void add_task(TaskControlBlock* task) = 0;
    virtual void remove_task(TaskControlBlock* task) = 0;
    virtual TaskControlBlock* pick_next() = 0;
    virtual void on_tick(TaskControlBlock* current) = 0;
};

class FIFOScheduler : public Scheduler
{
    std::vector<TaskControlBlock*> ready_queue_;
    uint32_t time_quantum_;
    uint32_t current_tick_;
    size_t current_index_;
    public:
    explicit FIFOScheduler(uint32_t time_quantum = 3);
    std::string name() const override;
    SchedAlgo algo() const override;
    void add_task(TaskControlBlock* task) override;
    void remove_task(TaskControlBlock* task) override;
    TaskControlBlock* pick_next() override;
    void on_tick(TaskControlBlock* current) override;
};

class PriorityScheduler :public Scheduler
{
    std::vector<TaskControlBlock*> ready_queue_[16];
    uint8_t highest_ready_;
    public:
    PriorityScheduler();
    std::string name() const override;
    SchedAlgo algo() const override;
    void add_task(TaskControlBlock* task) override;
    void remove_task(TaskControlBlock* task) override;
    TaskControlBlock* pick_next() override;
    void on_tick(TaskControlBlock* current) override;
};

#endif