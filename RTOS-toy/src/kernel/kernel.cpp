#include "../../include/kernel/kernel.h"

Kernel& Kernel::instance() {
    static Kernel inst;
    return inst;
}

void Kernel::init(Scheduler* sched) {
    scheduler_ = sched;
    tick_ = 0;
    current_task_ = nullptr;
    all_tasks_.clear();
}

void Kernel::create_task(TaskControlBlock* task) {
    all_tasks_.push_back(task);
    scheduler_->add_task(task);
}

bool Kernel::tick() {
    TaskControlBlock* next = scheduler_->pick_next();
    if (next == nullptr) {
        tick_++;
        return check_alive();
    }

    // Previous task is no longer running
    if (current_task_ != nullptr && current_task_ != next) {
        if (current_task_->state == TaskState::RUNNING) {
            current_task_->state = TaskState::READY;
        }
    }

    // Context switch to new task
    if (next != current_task_) {
        next->state = TaskState::RUNNING;
        next->context_switch_count++;
        current_task_ = next;
    } else {
        // Same task continues, ensure it's marked RUNNING
        next->state = TaskState::RUNNING;
    }

    // Run one step of the task's entry function
    if (next->entry) {
        next->entry();
    }

    // Account for this tick
    next->total_ticks_run++;

    // Let scheduler update its internal state (e.g. time-slice counter)
    scheduler_->on_tick(current_task_);

    tick_++;

    // If task is still RUNNING after entry returns, put it back to READY
    if (current_task_->state == TaskState::RUNNING) {
        current_task_->state = TaskState::READY;
    }

    // If the current task was blocked, current_task_ becomes stale;
    // set it to null so next tick re-picks properly
    if (current_task_->state == TaskState::BLOCKED) {
        current_task_ = nullptr;
    }

    return check_alive();
}

bool Kernel::check_alive() {
    if (all_tasks_.empty()) return false;
    for (auto* t : all_tasks_) {
        if (t->state != TaskState::TERMINATED) return true;
    }
    return false;
}

void Kernel::block_task(TaskControlBlock* task) {
    task->state = TaskState::BLOCKED;
    scheduler_->remove_task(task);
}

void Kernel::wakeup_task(TaskControlBlock* task) {
    task->state = TaskState::READY;
    task->blocked_on = nullptr;
    scheduler_->add_task(task);
}

Scheduler* Kernel::scheduler() const { return scheduler_; }
uint32_t Kernel::current_tick() const { return tick_; }
TaskControlBlock* Kernel::current_task() const { return current_task_; }
const std::vector<TaskControlBlock*>& Kernel::all_tasks() const { return all_tasks_; }
