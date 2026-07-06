#include "../include/kernel/kernel.h"
#include "../include/kernel/task.h"
#include "../include/kernel/scheduler.h"
#include "../include/scenarios/demo_scenarios.h"
#include <iostream>

int main() {
    // Set console to UTF-8 on Windows
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    std::cout << "\n"
              << "  ╔══════════════════════════════════════════╗\n"
              << "  ║       RTOS-Toy  Scheduler Demo          ║\n"
              << "  ║   A Toy Embedded RTOS Kernel in C++     ║\n"
              << "  ╚══════════════════════════════════════════╝\n\n";

    std::cout << "  Select demo scenario:\n\n";
    std::cout << "    [1] FIFO Round-Robin\n";
    std::cout << "        3 equal-priority tasks, time-sliced\n\n";
    std::cout << "    [2] Priority Preemptive\n";
    std::cout << "        3 tasks at priority 1, 3, 5\n";
    std::cout << "        Higher priority preempts lower\n\n";
    std::cout << "    [3] Producer-Consumer (Semaphore)\n";
    std::cout << "        Producer signals, Consumer waits\n\n";
    std::cout << "    [4] Priority Inheritance (Mutex)\n";
    std::cout << "        Low holds mutex, High waits → Low boosted\n\n";
    std::cout << "  Enter choice (1-4): ";

    int choice;
    std::cin >> choice;

    switch (choice) {
        case 1: run_scenario_fifo(); break;
        case 2: run_scenario_priority(); break;
        case 3: run_scenario_producer_consumer(); break;
        case 4: run_scenario_priority_inheritance(); break;
        default:
            std::cout << "Invalid choice, running FIFO.\n";
            run_scenario_fifo();
            break;
    }

    std::cout << "\n  Demo finished. Press Enter to exit...";
    std::cin.ignore();
    std::cin.get();
    return 0;
}
