#include "../../include/kernel/ipc.h"
#include "../../include/kernel/kernel.h"

// ───── Semaphore ──────────────────────────────────

Semaphore::Semaphore(int32_t initial_count)
    : count(initial_count)
{}

bool Semaphore::wait(TaskControlBlock* task, uint32_t /*tick*/) {
    if (count > 0) {
        count--;
        return true;
    }
    waiting_tasks.push_back(task);
    Kernel::instance().block_task(task);
    return false;
}

bool Semaphore::signal(TaskControlBlock* /*task*/) {
    count++;
    if (!waiting_tasks.empty()) {
        auto* woken = waiting_tasks.front();
        waiting_tasks.erase(waiting_tasks.begin());
        Kernel::instance().wakeup_task(woken);
        return true;
    }
    return false;
}

// ───── Mutex (with Priority Inheritance) ──────────

Mutex::Mutex()
    : owner(nullptr)
    , original_priority(0)
{}

bool Mutex::lock(TaskControlBlock* task, uint32_t /*tick*/) {
    if (owner == nullptr) {
        owner = task;
        original_priority = task->priority;
        return true;
    }

    // Priority inheritance: boost owner if waiter has higher priority
    if (task->priority > owner->priority) {
        owner->priority = task->priority;
    }

    waiting_tasks.push_back(task);
    Kernel::instance().block_task(task);
    return false;
}

bool Mutex::unlock(TaskControlBlock* task) {
    if (owner != task) return false;

    // Restore original priority
    owner->priority = original_priority;

    if (!waiting_tasks.empty()) {
        // Find highest priority waiter
        TaskControlBlock* next = nullptr;
        uint8_t highest = 0;
        size_t idx = 0;
        for (size_t i = 0; i < waiting_tasks.size(); i++) {
            if (waiting_tasks[i]->priority > highest) {
                highest = waiting_tasks[i]->priority;
                next = waiting_tasks[i];
                idx = i;
            }
        }

        waiting_tasks.erase(waiting_tasks.begin() + static_cast<long>(idx));

        // Transfer ownership before waking so the new owner is already set
        owner = next;
        original_priority = next->priority;

        Kernel::instance().wakeup_task(next);
    } else {
        owner = nullptr;
    }
    return true;
}

// ───── MessageQueue ───────────────────────────────

MessageQueue::MessageQueue(uint32_t max_size)
    : max_size(max_size)
{}

bool MessageQueue::send(TaskControlBlock* task,
                        const std::string& data,
                        uint32_t tick) {
    if (messages.size() < max_size) {
        messages.push({data, tick});

        // Wake a waiting receiver if any
        if (!waiting_recv.empty()) {
            auto* woken = waiting_recv.front();
            waiting_recv.erase(waiting_recv.begin());
            Kernel::instance().wakeup_task(woken);
        }
        return true;
    }

    waiting_send.push_back(task);
    Kernel::instance().block_task(task);
    return false;
}

bool MessageQueue::recv(TaskControlBlock* task,
                        uint32_t tick,
                        Message& out) {
    if (!messages.empty()) {
        out = messages.front();
        messages.pop();

        // Wake a waiting sender if any
        if (!waiting_send.empty()) {
            auto* woken = waiting_send.front();
            waiting_send.erase(waiting_send.begin());
            Kernel::instance().wakeup_task(woken);
        }
        return true;
    }

    waiting_recv.push_back(task);
    Kernel::instance().block_task(task);
    return false;
}
