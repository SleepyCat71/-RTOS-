#include "../../include/kernel/scheduler.h"
#include <algorithm>

// ───── FIFOScheduler ──────────────────────────────

FIFOScheduler::FIFOScheduler(uint32_t time_quantum)
    : time_quantum_(time_quantum)
    , current_tick_(0)
    , current_index_(0)
{}

std::string FIFOScheduler::name() const {
    return "FIFO Round-Robin";
}

SchedAlgo FIFOScheduler::algo() const {
    return SchedAlgo::FIFO;
}

void FIFOScheduler::add_task(TaskControlBlock* task) {
    ready_queue_.push_back(task);
}

void FIFOScheduler::remove_task(TaskControlBlock* task) {
    auto it = std::find(ready_queue_.begin(), ready_queue_.end(), task);
    if (it == ready_queue_.end()) return;

    size_t idx = static_cast<size_t>(it - ready_queue_.begin());
    ready_queue_.erase(it);

    // Adjust index so we don't skip a task
    if (current_index_ > idx && current_index_ > 0) {
        current_index_--;
    }
    if (current_index_ >= ready_queue_.size() && !ready_queue_.empty()) {
        current_index_ = 0;
    }
}

TaskControlBlock* FIFOScheduler::pick_next() {
    if (ready_queue_.empty()) return nullptr;
    return ready_queue_[current_index_];
}

void FIFOScheduler::on_tick(TaskControlBlock* /*current*/) {
    current_tick_++;
    if (current_tick_ >= time_quantum_) {
        current_tick_ = 0;
        if (!ready_queue_.empty()) {
            current_index_ = (current_index_ + 1) % ready_queue_.size();
        }
    }
}

// ───── PriorityScheduler (Preemptive) ─────────────

PriorityScheduler::PriorityScheduler()
    : highest_ready_(0)
{}

std::string PriorityScheduler::name() const {
    return "Priority-Preemptive";
}

SchedAlgo PriorityScheduler::algo() const {
    return SchedAlgo::PRIORITY;
}

void PriorityScheduler::add_task(TaskControlBlock* task) {
    uint8_t p = std::min<uint8_t>(task->priority, 15);
    ready_queue_[p].push_back(task);
    if (p > highest_ready_) {
        highest_ready_ = p;
    }
}

void PriorityScheduler::remove_task(TaskControlBlock* task) {
    uint8_t p = std::min<uint8_t>(task->priority, 15);
    auto& q = ready_queue_[p];
    auto it = std::find(q.begin(), q.end(), task);
    if (it == q.end()) return;

    q.erase(it);

    // Recompute highest_ready_ if this bucket is now empty
    if (q.empty() && p == highest_ready_) {
        for (int i = 15; i >= 0; i--) {
            if (!ready_queue_[i].empty()) {
                highest_ready_ = static_cast<uint8_t>(i);
                return;
            }
        }
        highest_ready_ = 0;
    }
}

TaskControlBlock* PriorityScheduler::pick_next() {
    auto& q = ready_queue_[highest_ready_];
    if (q.empty()) return nullptr;
    return q.front();
}

void PriorityScheduler::on_tick(TaskControlBlock* /*current*/) {
    // Preemptive: no time-slice rotation needed.
    // Re-pick happens every tick via Kernel::tick().
}
