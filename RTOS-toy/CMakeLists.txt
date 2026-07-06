cmake_minimum_required(VERSION 3.16)
project(rtos-toy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Sources ────────────────────────────────────────
set(KERNEL_SOURCES
    src/kernel/task.cpp
    src/kernel/ipc.cpp
    src/kernel/scheduler.cpp
    src/kernel/kernel.cpp
)

set(VIZ_SOURCES
    src/viz/console_viz.cpp
)

set(DEMO_SOURCES
    src/demo_scenarios.cpp
)

set(ALL_SOURCES
    src/main.cpp
    ${KERNEL_SOURCES}
    ${VIZ_SOURCES}
    ${DEMO_SOURCES}
)

# ── Executable ────────────────────────────────────
add_executable(rtos-toy ${ALL_SOURCES})

# ── Include directories ───────────────────────────
target_include_directories(rtos-toy PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ── Link threading library (for sleep) ────────────
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(rtos-toy PRIVATE Threads::Threads)
endif()
