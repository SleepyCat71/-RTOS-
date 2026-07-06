#include "../../include/kernel/task.h"

TaskControlBlock::TaskControlBlock(const std::string& name,
                                   std::function<void()> entry,
                                   uint8_t priority)
    : name(name)
    , entry(std::move(entry))
    , priority(priority)
    , state(TaskState::READY)
    , total_ticks_run(0)
    , context_switch_count(0)
    , blocked_on(nullptr)
{}
