# Mini OS Kernel Simulator: Command-Line Guide

All commands must be run from the root `cep_os` directory using PowerShell or Command Prompt.
**Base Command:** `.\build\os_simulator.exe`

---

## 1. Simulation Modes (`--mode`)
By default, running the program without arguments will execute `--mode full` with textual logs.

| Command | What it does |
| :--- | :--- |
| `--mode scheduling` | Runs only the CPU Scheduler simulation. |
| `--mode memory` | Runs the Page Fault & Memory Replacement demo. |
| `--mode sync` | Demonstrates Mutex and Semaphore synchronization handling. |
| `--mode compare` | Runs the workload against FCFS, Round Robin, and Priority back-to-back and outputs comparison tables. |
| `--mode full` | *(Default)* Runs all subsystems simultaneously. |

*Tip: Use `--quiet` with any mode (e.g., `.\build\os_simulator.exe --mode compare --quiet`) to hide the massive tick-by-tick logs and only see the final blue summary tables.*

---

## 2. Interactive Dashboard (`--interactive`)
Adding `--interactive` transforms the simulator from a fast text-logger into a live, animated Terminal User Interface (TUI). 

**Command:** `.\build\os_simulator.exe --interactive --mode scheduling`

**TUI Controls (Press while running):**
*   `SPACE` : Pause / Resume the simulation
*   `+` : Speed up (decreases tick delay by 50ms)
*   `-` : Slow down (increases tick delay by 50ms)
*   `q` : Quit immediately

---

## 3. Workload Configurations (`--config`)
You can load predefined workload profiles to test how the OS reacts to different scenarios.

**Command:** `.\build\os_simulator.exe --config config\cpu_bound.cfg`

**Available Profiles:**
*   `config\default.cfg` *(Default)* - Balanced mix.
*   `config\cpu_bound.cfg` - Processes do heavy computation, rarely yielding for I/O.
*   `config\io_bound.cfg` - Processes do quick bursts, then wait long times for I/O.
*   `config\mixed.cfg` - A highly varied, realistic 10-process workload.

---

## 4. Customizing Parameters
If you don't use a `--config` file, you can manually override any parameter directly in the console.

### CPU Scheduling
| Parameter | Options | Default if omitted |
| :--- | :--- | :--- |
| `--scheduler` | `fcfs`, `rr`, `priority` | `rr` (Round Robin) |
| `--quantum` | Any integer (e.g., `2`, `10`) | `4` |

*Example: Run Priority scheduler:*
`.\build\os_simulator.exe --interactive --mode scheduling --scheduler priority`

### Memory & Paging
| Parameter | Options | Default if omitted |
| :--- | :--- | :--- |
| `--replacement` | `fifo`, `lru` | `lru` (Least Recently Used) |
| `--frames` | Any integer (Physical Memory size) | `16` |

*Example: Run FIFO with very constrained memory (only 8 frames):*
`.\build\os_simulator.exe --interactive --mode memory --replacement fifo --frames 8`

### Processes
| Parameter | Options | Default if omitted |
| :--- | :--- | :--- |
| `--processes` | Any integer | `5` |

*Example: Simulate 20 processes using Round Robin with a quantum of 2:*
`.\build\os_simulator.exe --interactive --processes 20 --quantum 2`

---

## 5. The "God Mode" Command
If you want to customize absolutely everything at once, you can chain the arguments:

```powershell
.\build\os_simulator.exe --interactive --mode scheduling --scheduler rr --quantum 5 --replacement lru --frames 32 --processes 12
```
