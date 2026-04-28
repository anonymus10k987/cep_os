# Mini OS Kernel Simulator
### CS-330: Operating Systems — Semester Design Project (CEP)

A fully functional **Mini OS Kernel Simulator** written in C++20 that demonstrates core operating system concepts through a live, interactive terminal dashboard.

---

## Features

| Subsystem | What's Simulated |
|-----------|-----------------|
| **CPU Scheduling** | FCFS, Round Robin, Priority (preemptive & non-preemptive) |
| **Process Management** | PCB lifecycle: NEW → READY → RUNNING → WAITING → TERMINATED |
| **Memory Management** | Paging with FIFO and LRU page replacement |
| **Synchronization** | Mutex (ownership + wait queue), Counting Semaphore (P/V) |
| **Interactive TUI** | Live dashboard: CPU bar, process table, memory heat-map, metrics |

---

## Quick Start

### Option A — Run the pre-built binary (Windows only)

No compiler needed. Just clone and run:

```bat
git clone https://github.com/anonymus10k987/cep_os.git
cd cep_os

REM Interactive animated dashboard
build\os_simulator.exe --interactive --mode scheduling

REM Compare all 3 schedulers
build\os_simulator.exe --mode compare --quiet

REM Memory demo (FIFO vs LRU)
build\os_simulator.exe --mode memory

REM Synchronization demo (race condition, mutex, semaphore)
build\os_simulator.exe --mode sync
```

### Option B — Build from source (Windows + MinGW/GCC)

**Prerequisites:** [MSYS2](https://www.msys2.org/) with `g++` supporting C++20

```bat
git clone https://github.com/anonymus10k987/cep_os.git
cd cep_os
.\build.bat
build\os_simulator.exe --interactive --mode scheduling
```

---

## Interactive TUI Controls

When running with `--interactive`:

| Key | Action |
|-----|--------|
| `SPACE` or `P` | Pause / Resume |
| `+` | Speed up (−50 ms/tick) |
| `-` | Slow down (+50 ms/tick) |
| `Q` | Quit |

---

## All CLI Options

```
build\os_simulator.exe [options]

  --mode        full | compare | sync | memory | scheduling  (default: full)
  --interactive  Auto-advancing TUI dashboard
  --scheduler   fcfs | rr | priority                        (default: rr)
  --quantum     Time quantum for Round Robin                 (default: 4)
  --replacement fifo | lru                                   (default: lru)
  --processes   Number of processes to simulate             (default: 5)
  --frames      Number of physical memory frames            (default: 16)
  --config      Path to a .cfg workload profile
  --quiet       Suppress per-tick console logging
  --help        Show help
```

### Example Commands

```bat
REM Interactive with mixed workload profile
build\os_simulator.exe --interactive --config config\mixed.cfg --mode scheduling

REM CPU-bound workload comparison
build\os_simulator.exe --config config\cpu_bound.cfg --mode compare --quiet

REM I/O-bound, priority scheduler, 8 processes
build\os_simulator.exe --scheduler priority --processes 8 --mode scheduling --quiet
```

---

## Workload Profiles (`config/`)

| File | Description |
|------|-------------|
| `default.cfg` | Balanced baseline (5 processes, RR, 16 frames) |
| `cpu_bound.cfg` | Long CPU bursts, minimal I/O |
| `io_bound.cfg` | Short bursts, heavy I/O blocking |
| `mixed.cfg` | 10 processes, wide burst ranges — realistic multi-user load |

---

## Project Structure

```
cep_os/
├── build/                        Pre-compiled Windows binary
├── config/                       Workload scenario .cfg files
├── docs/                         Technical report, diagrams, slides
├── analysis/                     Python scripts for result graphs
├── include/
│   ├── sim_kernel.h              Central simulation engine + TUI
│   ├── kernel/
│   │   ├── pcb.h                 Process Control Block
│   │   ├── tcb.h                 Thread Control Block
│   │   ├── scheduler.h           Abstract scheduler interface
│   │   ├── fcfs_scheduler.h
│   │   ├── round_robin_scheduler.h
│   │   ├── priority_scheduler.h
│   │   ├── process_manager.h
│   │   ├── memory/
│   │   │   ├── frame_allocator.h
│   │   │   ├── page_table.h
│   │   │   └── memory_manager.h  FIFO + LRU replacement policies
│   │   └── sync/
│   │       ├── mutex.h
│   │       └── semaphore.h
│   └── utils/
│       ├── config.h              .cfg file parser
│       ├── logger.h              Console + CSV structured logger
│       └── metrics.h             Metrics collector + JSON export
├── src/
│   └── main.cpp                  Entry point + demo modes
└── build.bat                     Build script (g++ C++20)
```

---

## Output Files

After running, logs are saved to `output/logs/`:

| File | Contents |
|------|----------|
| `scheduling_log.csv` | Per-tick scheduling events |
| `memory_log.csv` | Page fault and replacement events |
| `scheduling_metrics.json` | Avg wait, turnaround, response, CPU utilization |
| `memory_metrics.json` | Fault rate, frame utilization |

---

## Requirements (to build from source)

- Windows 10/11
- [MSYS2](https://www.msys2.org/) — install with: `pacman -S mingw-w64-ucrt-x86_64-gcc`
- g++ with C++20 support (`g++ --version` should show ≥ 12)

---

*CS-330 Operating Systems — Semester Design Project*
