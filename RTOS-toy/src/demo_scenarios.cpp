#include "../include/kernel/kernel.h"
#include "../include/kernel/task.h"
#include "../include/kernel/scheduler.h"
#include "../include/kernel/ipc.h"
#include "../include/viz/console_viz.h"
#include "../include/scenarios/demo_scenarios.h"
#include <thread>
#include <chrono>
#include <iostream>

// ───── Global visualization and IPC object lists ──

static ConsoleViz g_viz;
static std::vector<Semaphore*> g_sems;
static std::vector<Mutex*> g_mutexes;
static std::vector<MessageQueue*> g_queues;

// Per-tick delay so the animation is visible (ms)
static constexpr uint32_t TICK_MS = 200;

static void sleep_tick() {
    std::this_thread::sleep_for(std::chrono::milliseconds(TICK_MS));
}

// Update the gantt chart for a single tick
static void update_gantt(TaskControlBlock* current,
                         const std::vector<TaskControlBlock*>& tasks) {
    for (auto* t : tasks) {
        if (t->state == TaskState::TERMINATED) continue;
        if (t == current && current->state == TaskState::RUNNING) {
            g_viz.record_gantt(t, '\xDB');   // ▓ running
        } else if (t->state == TaskState::BLOCKED) {
            g_viz.record_gantt(t, '\xB2');   // ▒ blocked
        } else {
            g_viz.record_gantt(t, '\xB0');   // ░ idle/ready
        }
    }
}

// Main simulation loop shared by all scenarios
static void run_simulation(uint32_t total_ticks) {
    Kernel& k = Kernel::instance();

    for (uint32_t i = 0; i < total_ticks; i++) {
        if (!k.tick()) break;  // all tasks terminated

        auto* current = k.current_task();
        auto& tasks = k.all_tasks();

        update_gantt(current, tasks);

        g_viz.render(k.current_tick(), tasks, current,
                     k.scheduler(), g_sems, g_mutexes, g_queues);

        sleep_tick();
    }
}

// ───── Demo task functions ────────────────────────

// Helper: get the current task's private step counter
// We store the counter inside a global array indexed by task name hash.
// Simpler approach: use a static map from task name to step count.
#include <map>
static std::map<std::string, int> g_task_steps;

static void step_and_maybe_finish(int max_steps) {
    auto& step = g_task_steps[Kernel::instance().current_task()->name];
    step++;
    if (step >= max_steps) {
        Kernel::instance().current_task()->state = TaskState::TERMINATED;
    }
}

// ───── Scenario 1: FIFO Round-Robin ───────────────

static void fifo_task_sensor() {
    step_and_maybe_finish(8);
}

static void fifo_task_controller() {
    step_and_maybe_finish(8);
}

static void fifo_task_reporter() {
    step_and_maybe_finish(8);
}

void run_scenario_fifo() {
    g_task_steps.clear();
    g_sems.clear();
    g_mutexes.clear();
    g_queues.clear();

    FIFOScheduler sched(3);
    Kernel& k = Kernel::instance();
    k.init(&sched);

    k.create_task(new TaskControlBlock("Sensor",     fifo_task_sensor,     5));
    k.create_task(new TaskControlBlock("Controller", fifo_task_controller, 5));
    k.create_task(new TaskControlBlock("Reporter",   fifo_task_reporter,   5));

    std::cout << "\n  Scenario 1: FIFO Round-Robin\n";
    std::cout << "  3 tasks at priority 5, time quantum = 3 ticks\n\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    run_simulation(60);
}

// ───── Scenario 2: Priority Preemptive ────────────

static void prio_task_high() {
    step_and_maybe_finish(10);
}

static void prio_task_medium() {
    step_and_maybe_finish(10);
}

static void prio_task_low() {
    step_and_maybe_finish(10);
}

void run_scenario_priority() {
    g_task_steps.clear();
    g_sems.clear();
    g_mutexes.clear();
    g_queues.clear();

    PriorityScheduler sched;
    Kernel& k = Kernel::instance();
    k.init(&sched);

    k.create_task(new TaskControlBlock("Low",    prio_task_low,   1));
    k.create_task(new TaskControlBlock("Medium", prio_task_medium, 3));
    k.create_task(new TaskControlBlock("High",   prio_task_high,  5));

    std::cout << "\n  Scenario 2: Priority Preemptive\n";
    std::cout << "  Low(prio=1), Medium(prio=3), High(prio=5)\n";
    std::cout << "  High-priority tasks preempt lower ones immediately.\n\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    run_simulation(60);
}

// ───── Scenario 3: Producer-Consumer ──────────────

static Semaphore sem_prod_cons(0);

static void producer_task() {
    auto* k = &Kernel::instance();
    auto* self = k->current_task();

    static int prod_count = 0;
    prod_count++;

    if (prod_count <= 10) {
        // Signal consumer that data is available
        sem_prod_cons.signal(self);
    } else {
        self->state = TaskState::TERMINATED;
    }
}

static void consumer_task() {
    auto* k = &Kernel::instance();
    auto* self = k->current_task();

    static int cons_count = 0;
    cons_count++;

    if (cons_count <= 10) {
        // Wait for producer
        sem_prod_cons.wait(self, k->current_tick());
    } else {
        self->state = TaskState::TERMINATED;
    }
}

void run_scenario_producer_consumer() {
    g_task_steps.clear();
    g_sems.clear();
    g_mutexes.clear();
    g_queues.clear();

    sem_prod_cons = Semaphore(0);
    g_sems.push_back(&sem_prod_cons);

    FIFOScheduler sched(2);
    Kernel& k = Kernel::instance();
    k.init(&sched);

    k.create_task(new TaskControlBlock("Producer",  producer_task,  5));
    k.create_task(new TaskControlBlock("Consumer",  consumer_task,  5));

    std::cout << "\n  Scenario 3: Producer-Consumer (Semaphore)\n";
    std::cout << "  Producer signals semaphore each tick;\n";
    std::cout << "  Consumer waits on semaphore, blocks when empty.\n\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    run_simulation(80);
}

// ───── Scenario 4: Priority Inheritance ───────────

static Mutex shared_mutex;

static void inherit_task_low() {
    auto* k = &Kernel::instance();
    auto* self = k->current_task();

    static int phase = 0;
    phase++;

    if (phase == 1) {
        // Low-priority task acquires the mutex first
        shared_mutex.lock(self, k->current_tick());
    } else if (phase >= 6 && phase <= 10) {
        // Hold the mutex for a few ticks while high-priority tasks wait
        // (simulating slow work)
    } else if (phase == 11) {
        // Release the mutex
        shared_mutex.unlock(self);
        self->state = TaskState::TERMINATED;
    }
}

static void inherit_task_high() {
    auto* k = &Kernel::instance();
    auto* self = k->current_task();

    static int phase = 0;
    phase++;

    if (phase == 2) {
        // Try to lock the mutex (held by Low), should block
        shared_mutex.lock(self, k->current_tick());
    } else if (phase >= 3 && phase <= 12) {
        // Once we get the mutex, do work
    } else if (phase == 13) {
        shared_mutex.unlock(self);
        self->state = TaskState::TERMINATED;
    }
}

static void inherit_task_medium() {
    static int phase = 0;
    phase++;

    if (phase >= 10 && phase <= 20) {
        // Medium priority task tries to run - but with priority inheritance,
        // Low gets boosted to High's priority, so Medium won't preempt Low.
        // This demonstrates priority inheritance preventing inversion.
    }

    if (phase >= 25) {
        Kernel::instance().current_task()->state = TaskState::TERMINATED;
    }
}

void run_scenario_priority_inheritance() {
    g_task_steps.clear();
    g_sems.clear();
    g_mutexes.clear();
    g_queues.clear();

    shared_mutex = Mutex();
    g_mutexes.push_back(&shared_mutex);

    PriorityScheduler sched;
    Kernel& k = Kernel::instance();
    k.init(&sched);

    // Low(1) holds mutex first, High(5) tries to lock it → Low inherits priority 5
    k.create_task(new TaskControlBlock("LowWorker",   inherit_task_low,   1));
    k.create_task(new TaskControlBlock("HighWorker",  inherit_task_high,  5));
    k.create_task(new TaskControlBlock("MidInterrupt", inherit_task_medium, 3));

    std::cout << "\n  Scenario 4: Priority Inheritance\n";
    std::cout << "  Low(prio=1) holds mutex → High(prio=5) waits → Low boosted to 5\n";
    std::cout << "  Medium(prio=3) cannot preempt Low (inherited prio=5)\n\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    run_simulation(80);
}
