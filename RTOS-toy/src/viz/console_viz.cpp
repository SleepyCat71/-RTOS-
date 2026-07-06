#include "../../include/viz/console_viz.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

ConsoleViz::ConsoleViz() {
#ifdef _WIN32
    // Enable ANSI escape codes on Windows 10+
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) {
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

void ConsoleViz::render(uint32_t tick,
                        const std::vector<TaskControlBlock*>& tasks,
                        TaskControlBlock* current,
                        Scheduler* sched,
                        const std::vector<Semaphore*>& sems,
                        const std::vector<Mutex*>& mutexes,
                        const std::vector<MessageQueue*>& queues) {
    draw_header(tick, sched->algo());
    draw_task_table(tasks, current);
    draw_gantt(tasks);
    draw_ipc_status(sems, mutexes, queues);
}

void ConsoleViz::clear_screen() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);
    DWORD written;
    COORD topLeft = {0, 0};
    DWORD size = static_cast<DWORD>(csbi.dwSize.X) * csbi.dwSize.Y;
    FillConsoleOutputCharacterA(h, ' ', size, topLeft, &written);
    FillConsoleOutputAttribute(h, csbi.wAttributes, size, topLeft, &written);
    SetConsoleCursorPosition(h, topLeft);
#else
    std::cout << "\033[2J\033[H";
#endif
}

void ConsoleViz::draw_header(uint32_t tick, SchedAlgo algo) {
    clear_screen();

    const char* algo_str = (algo == SchedAlgo::FIFO)
        ? "FIFO Round-Robin"
        : "Priority-Preemptive";

    std::cout << "\n"
              << "  ╔══════════════════════════════════════════════════╗\n"
              << "  ║     RTOS-Toy  \x1b[36mTiny Kernel Scheduler Demo\x1b[0m     ║\n"
              << "  ╠══════════════════════════════════════════════════╣\n";

    std::cout << "  ║  Tick: " << std::setw(4) << tick
              << "   Algo: " << std::setw(22) << std::left
              << algo_str << std::right << "║\n";

    std::cout << "  ╚══════════════════════════════════════════════════╝\n\n";
}

void ConsoleViz::draw_task_table(const std::vector<TaskControlBlock*>& tasks,
                                 TaskControlBlock* current) {
    std::cout << "  ┌─────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "  │ Task               State      Prio   CPU%%     Ticks    CtxSw  Status   │\n";
    std::cout << "  ├─────────────────────────────────────────────────────────────────────────┤\n";

    uint32_t total_ticks = 0;
    for (auto* t : tasks) {
        total_ticks += t->total_ticks_run;
    }

    for (auto* t : tasks) {
        if (t->state == TaskState::TERMINATED) continue;

        const char* color_start = "";
        const char* color_end = "\033[0m";
        const char* state_str = "";

        if (t == current && t->state == TaskState::RUNNING) {
            color_start = "\033[32m";  // green
            state_str = "RUNNING";
        } else if (t->state == TaskState::BLOCKED) {
            color_start = "\033[33m";  // yellow
            state_str = "BLOCKED";
        } else if (t->state == TaskState::READY) {
            state_str = "READY";
        } else {
            color_start = "\033[31m";  // red
            state_str = "TERM";
        }

        double cpu_pct = (total_ticks > 0)
            ? (static_cast<double>(t->total_ticks_run) / total_ticks) * 100.0
            : 0.0;

        std::ostringstream status;
        if (t->state == TaskState::BLOCKED && t->blocked_on != nullptr) {
            status << "[waiting]";
        } else if (t->state == TaskState::RUNNING) {
            status << "[active]";
        } else {
            status << "[ready]  ";
        }

        std::cout << "  " << color_start
                  << "│ " << std::setw(19) << std::left << t->name
                  << std::setw(9) << state_str
                  << " " << std::setw(3) << static_cast<int>(t->priority)
                  << "  " << std::setw(5) << std::fixed << std::setprecision(1) << cpu_pct
                  << " " << std::setw(7) << t->total_ticks_run
                  << " " << std::setw(6) << t->context_switch_count
                  << " " << std::setw(10) << std::left << status.str() << std::right
                  << " │" << color_end << "\n";
    }
    std::cout << "  └─────────────────────────────────────────────────────────────────────────┘\n";
}

void ConsoleViz::draw_gantt(const std::vector<TaskControlBlock*>& tasks) {
    constexpr int gantt_width = 44;

    std::cout << "\n  ┌─── Gantt (\x1b[32m\u2593\x1b[0mR \x1b[33m\u2592\x1b[0mB \x1b[37m\u2591\x1b[0mI) "
              << "────────────────────────────────────┐\n";

    for (auto* t : tasks) {
        if (t->state == TaskState::TERMINATED) continue;

        std::cout << "  │ " << std::setw(14) << std::left << t->name << " ";

        bool found = false;
        for (auto& g : gantt_lines_) {
            if (g.name == t->name) {
                std::string display = g.line;
                if (static_cast<int>(display.length()) > gantt_width) {
                    display = display.substr(display.length() - gantt_width);
                }
std::cout << "\033[37m";
        for (int i = 0; i < gantt_width - static_cast<int>(display.length()); i++) {
            std::cout << "\u2591";
        }
        std::cout << display << "\033[0m";
        found = true;
        break;
            }
        }
        if (!found) {
            std::cout << "\033[37m";
            for (int i = 0; i < gantt_width; i++) {
                std::cout << "\u2591";
            }
            std::cout << "\033[0m";
            GanttLine gl;
            gl.name = t->name;
            gl.line = "";
            const_cast<ConsoleViz*>(this)->gantt_lines_.push_back(gl);
        }
        std::cout << " │\n";
    }
    std::cout << "  └─────────────────────────────────────────────────────────────────────────┘\n";
}

void ConsoleViz::draw_ipc_status(const std::vector<Semaphore*>& sems,
                                 const std::vector<Mutex*>& mutexes,
                                 const std::vector<MessageQueue*>& queues) {
    std::cout << "\n  ┌─── IPC Resources ────────────────────────────────────────────────────┐\n";

    if (sems.empty() && mutexes.empty() && queues.empty()) {
        std::cout << "  │  (none)                                                    │\n";
    }

    for (auto* s : sems) {
        std::cout << "  │  \x1b[33mSemaphore\x1b[0m  count=" << s->count
                  << "  waiting=" << s->waiting_tasks.size();
        // Pad to fixed width
        std::cout << "                                  │\n";
    }
    for (auto* m : mutexes) {
        std::string owner_name = m->owner ? m->owner->name : "(free)";
        std::cout << "  │  \x1b[33mMutex\x1b[0m      owner=" << std::setw(14) << std::left
                  << owner_name << std::right
                  << "  waiting=" << m->waiting_tasks.size();
        std::cout << "                           │\n";
    }
    for (auto* q : queues) {
        std::cout << "  │  \x1b[33mMsgQueue\x1b[0m   " << q->messages.size() << "/" << q->max_size
                  << "  send_wait=" << q->waiting_send.size()
                  << "  recv_wait=" << q->waiting_recv.size();
        // Show first message if any
        if (!q->messages.empty()) {
            std::cout << "  front=\"" << q->messages.front().data << "\"";
        }
        std::cout << "    │\n";
    }
    std::cout << "  └─────────────────────────────────────────────────────────────────────────┘\n";

    // Flush so output appears immediately
    std::cout.flush();
}

void ConsoleViz::record_gantt(TaskControlBlock* task, char c) {
    for (auto& g : gantt_lines_) {
        if (g.name == task->name) {
            g.line += c;
            return;
        }
    }
    // First time seeing this task
    GanttLine gl;
    gl.name = task->name;
    gl.line = std::string(1, c);
    gantt_lines_.push_back(gl);
}
