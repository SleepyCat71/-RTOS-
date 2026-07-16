# RTOS-Toy — 微型嵌入式实时内核模拟器

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)]()

这是一个微型 RTOS 内核模拟器，在桌面环境中通过 tick 驱动和协作式任务切换，完整复现了嵌入式实时内核的核心调度与 IPC 机制。不需要开发板，不需要交叉编译，任何有 C++17 编译器的平台都能运行。

```
  ╔══════════════════════════════════════════════════╗
  ║     RTOS-Toy  Tiny Kernel Scheduler Demo         ║
  ╠══════════════════════════════════════════════════╣
  ║  Tick:   42   Algorithm: Priority-Preemptive     ║
  ╚══════════════════════════════════════════════════╝

  ┌─────────────────────────────────────────────────────────────────────────┐
  │ Task               State      Prio   CPU%     Ticks    CtxSw  Status   │
  ├─────────────────────────────────────────────────────────────────────────┤
  │ Sensor             RUNNING     5     48.0%      20       7     [active] │
  │ Controller         BLOCKED     2      0.0%       0       0     [waiting]│
  │ Reporter           READY       1     52.0%      22       8     [ready]  │
  └─────────────────────────────────────────────────────────────────────────┘

  ┌─── Gantt (▓R ▒B ░I) ────────────────────────────────────────────────────┐
  │ Sensor       ░░░░░░░░░░▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░                  │
  │ Controller   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒                │
  │ Reporter     ▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░                │
  └─────────────────────────────────────────────────────────────────────────┘

  ┌─── IPC Resources ──────────────────────────────────────────────────────┐
  │  Semaphore  count=2  waiting=0                                         │
  │  Mutex      owner=Controller  waiting=1                                │
  │  MsgQueue   3/8  send_wait=0  recv_wait=0  front="hello"              │
  └─────────────────────────────────────────────────────────────────────────┘
```


## Features

-包含FIFO 时间片轮转 + 优先级可抢占调度两种调度算法\n
-包含信号量、互斥锁（含优先级继承）、消息队列三种IPC原语
-实时终端可视化，包含ASCII 任务状态表 + 调度甘特图 + IPC 资源看板
-4个交互演示场景一键运行，直观对比不同调度策略
-纯 C++17 标准库，跨平台编译运行


### 核心模块
TaskControlBlock模块：task.h/cpp。负责任务名、入口函数、优先级、四态机、运行统计、阻塞来源                  |
FIFOScheduler模块：scheduler.h/cpp。负责时间片轮转，固定时间配额，环形就绪队列                       |
PriorityScheduler模块：scheduler.h/cpp。负责16级位图就绪队列，pick_next O(1)，高优先级抢占低优先级 |
Semaphore模块：ipc.h/cpp。负责P/V 操作，等待队列，阻塞/唤醒调度                               |
Mutex模块：ipc.h/cpp。负责互斥锁 + 优先级继承防止优先级反转                                    |
MessageQueue模块：ipc.h/cpp。负责固定容量 FIFO 队列，满阻塞发送方，空阻塞接收方                     |
Kernel模块：kernel.h/cpp。负责单例内核，tick 驱动主循环，block/wakeup 统一入口                 |
ConsoleViz模块：console_viz.h/cpp。负责ANSI 清屏 + 状态表 + 甘特图 + IPC 面板             |


## 演示场景详解
### 场景 1: 时间片轮转

三个优先级相同的任务（Sensor/Controller/Reporter），使用 FIFOScheduler 时间片 = 3 ticks。每 tick 检查时间片计数器，用完则旋转 `current_index_` 指向下一个任务。

现象：甘特图中三个任务呈均匀交替的 ▓▓▓░░░ 三段式图案。

### 场景 2: 优先级抢占

三个任务优先级 1/3/5（Low/Medium/High），使用 PriorityScheduler。`pick_next()` 始终返回最高就绪优先级队首。只要 High(5) 就绪，Medium(3) 和 Low(1) 无法获得 CPU。

现象：High 几乎独占 CPU，Low 和 Medium 在 High 终止后才运行。

### 场景 3: 信号量

共享一个初始值为 0 的 Semaphore。Producer 每 tick `signal()`，Consumer 每 tick `wait()`。当 count=0 时 Consumer 阻塞（`block_task` 移出就绪队列），Producer `signal()` 后唤醒 Consumer（`wakeup_task` 重新加入就绪队列）。

现象：Consumer 状态在 READY 和 BLOCKED 之间交替，甘特图穿插 ▒（阻塞）和 ▓（运行）。

### 场景 4: 优先级反转
演示经典优先级反转问题及其解决：

1. Low(1) 获得 Mutex 锁
2. High(5) 请求同一锁 → 阻塞 → `Mutex::lock` 检测优先级差 → **Low 优先级提升至 5**
3. Medium(3) 就绪 → 调度器发现最高优先级是 5（Low 被提升了）→ **Medium 被压制**
4. Low 完成工作 → `unlock` → 恢复原始优先级 1 → 唤醒等待队列中优先级最高者（High）
5. High 获得锁 → 执行 → 释放 → Medium 终于获得 CPU

现象：Low 被提升后连续运行不受 Medium 干扰，甘特图中 Low 的执行段贯穿 High 阻塞的全程。


## 项目结构

```
rtos-toy/
├── CMakeLists.txt
├── include/
│   ├── kernel/
│   │   ├── task.h           # 任务控制块 TCB
│   │   ├── scheduler.h      # 调度器抽象 + 两种实现
│   │   ├── kernel.h         # 内核单例
│   │   └── ipc.h            # IPC 原语
│   ├── viz/
│   │   └── console_viz.h    # 可视化引擎
│   └── scenarios/
│       └── demo_scenarios.h # 场景声明
├── src/
│   ├── kernel/
│   │   ├── task.cpp
│   │   ├── scheduler.cpp
│   │   ├── kernel.cpp
│   │   └── ipc.cpp
│   ├── viz/
│   │   └── console_viz.cpp
│   ├── demo_scenarios.cpp
│   └── main.cpp
└── README.md
```

## License

MIT
