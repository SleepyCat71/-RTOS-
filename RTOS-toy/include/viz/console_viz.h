#ifndef RTOS_TOY_CONSOLE_VIZ_H
#define RTOS_TOY_CONSOLE_VIZ_H

#include <cstdint>
#include <string>
#include <vector>
#include "../kernel/task.h"
#include "../kernel/ipc.h"
#include "../kernel/scheduler.h"

class ConsoleViz {
public:
    ConsoleViz();

    void render(uint32_t tick,
                const std::vector<TaskControlBlock*>& tasks,
                TaskControlBlock* current,
                Scheduler* sched,
                const std::vector<Semaphore*>& sems,
                const std::vector<Mutex*>& mutexes,
                const std::vector<MessageQueue*>& queues);

    // Record one tick for gantt chart: c = '▓' RUNNING, '▒' BLOCKED, '░' READY
    void record_gantt(TaskControlBlock* task, char c);

private:
    void clear_screen();
    void draw_header(uint32_t tick, SchedAlgo algo);
    void draw_task_table(const std::vector<TaskControlBlock*>& tasks,
                         TaskControlBlock* current);
    void draw_gantt(const std::vector<TaskControlBlock*>& tasks);
    void draw_ipc_status(const std::vector<Semaphore*>& sems,
                         const std::vector<Mutex*>& mutexes,
                         const std::vector<MessageQueue*>& queues);

    struct GanttLine {
        std::string name;
        std::string line;
    };
    std::vector<GanttLine> gantt_lines_;
};

#endif
