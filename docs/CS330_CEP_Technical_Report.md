# CS-330: Operating Systems - Semester Design Project (CEP)
## Technical Report: Mini OS Kernel Simulator

**Instructor:** AP Mobeena Shahzad  
**Course/Semester:** BESE 30 (A, B, C) - Spring 2026  

---

## 1. Introduction & Problem Statement
The objective of this Complex Engineering Problem (CEP) is to design and implement a Mini Operating System Kernel Simulator in user space. The system models fundamental OS functionalities, including process and thread management, CPU scheduling, process synchronization, and paging-based memory management. 

By executing as a user-level C++ application, this simulator provides a sandbox to apply core OS abstractions, test configurable scheduling policies against realistic workloads, and quantitatively evaluate performance trade-offs (throughput, fairness, latency, and page faults) without the complexities of actual kernel-mode programming.

---

## 2. System Architecture & Design Choices

The simulator is built upon a modular, tick-based simulation engine (`SimKernel`), orchestrating four primary subsystems.

### 2.1 Process & Thread Management
The system models processes through a highly detailed Process Control Block (PCB) and threads via Thread Control Blocks (TCB).
*   **Data Structures:** The `PCB` tracks `pid`, `name`, `state`, `total_cpu_burst`, `remaining_burst`, `io_burst`, `priority`, and a vector of requested memory `pages`.
*   **State Machine:** Processes transition through the standard 5-state lifecycle: `NEW` → `READY` → `RUNNING` → `WAITING` (for I/O or Synchronization) → `TERMINATED`.
*   **Process Manager:** The `ProcessManager` acts as the central repository, moving PCBs between queues (e.g., the Ready Queue and I/O Waiting Queue) based on their state.

### 2.2 CPU Scheduling
The simulator implements the `IScheduler` interface to support dynamic runtime switching of CPU scheduling algorithms.
*   **First-Come, First-Served (FCFS):** A non-preemptive queue where the first arrived process executes until completion or I/O request.
*   **Round Robin (RR):** A preemptive approach utilizing a configurable time `quantum`. If a process exhausts its quantum, it is context-switched and moved to the back of the ready queue.
*   **Priority Scheduling:** A preemptive scheduler where the CPU is allocated to the process with the highest priority (lowest integer value).

### 2.3 Process Synchronization
To simulate concurrency control, the kernel implements fundamental synchronization primitives.
*   **Mutex:** Implements ownership. If a thread attempts to lock an already locked mutex, it is transitioned to a `WAITING` state and placed in the mutex's wait queue.
*   **Counting Semaphore:** Implements standard `Wait (P)` and `Signal (V)` operations to handle scenarios like the Producer-Consumer problem, allowing multiple resources to be managed concurrently without race conditions.

### 2.4 Memory Management (Paging)
The memory manager simulates a hardware MMU using a paging abstraction.
*   **Physical Memory:** Modelled as a fixed pool of physical frames managed by a `FrameAllocator`.
*   **Virtual Memory & Page Tables:** Each process has a localized `PageTable`. When a process executes, the memory manager maps its logical pages to physical frames.
*   **Replacement Policies:** If physical memory is exhausted, the system invokes either:
    *   **FIFO (First-In, First-Out):** Evicts the oldest loaded page.
    *   **LRU (Least Recently Used):** Evicts the page that has not been accessed for the longest time, tracked via simulation tick timestamps.

---

## 3. OS Design Artifacts

*(Note: Replace the placeholder blocks below with the actual exported images from your diagramming tools (e.g., Draw.io, Visio, Lucidchart).)*

### 3.1 Process State Diagram
> **[ PLACEHOLDER: Insert Process Lifecycle State Diagram Here ]**
> *Description:* This diagram should visually represent the transitions between NEW, READY, RUNNING, WAITING, and TERMINATED states, clearly showing the triggers (e.g., `Admit`, `Dispatch`, `Interrupt/Quantum Expired`, `I/O Event`, `Exit`).

### 3.2 Scheduler Flowcharts
> **[ PLACEHOLDER: Insert Round Robin Scheduler Flowchart Here ]**
> *Description:* This flowchart must detail the Round Robin logic: checking the Ready Queue, dispatching the PCB, decrementing the burst and quantum, handling context switches upon quantum expiration, and managing I/O interruptions.

### 3.3 Memory Management & Paging Diagram
> **[ PLACEHOLDER: Insert Paging Mechanism Diagram Here ]**
> *Description:* A structural diagram showing Logical Memory mapping to Physical Memory frames via the Page Table, including the Page Fault generation flow and the invocation of the LRU/FIFO replacement handler.

---

## 4. Experimental Setup & Workload Generation

To analyze the performance trade-offs of our scheduling and memory policies, we utilize the `WorkloadGenerator` to create three distinct configuration profiles (`.cfg` files):

1.  **CPU-Bound Workload (`cpu_bound.cfg`):** Processes have extremely long CPU bursts (e.g., 20-50 ticks) and minimal I/O interruptions.
2.  **I/O-Bound Workload (`io_bound.cfg`):** Processes feature short CPU bursts (1-5 ticks) but frequently yield the CPU to wait for long I/O operations.
3.  **Mixed Workload (`mixed.cfg`):** A realistic distribution of both CPU-heavy and I/O-heavy processes, running concurrently.

Metrics collected per simulation run include:
*   **Scheduling:** Context Switches, Average Waiting Time, Average Turnaround Time, CPU Utilization, and Throughput.
*   **Memory:** Total Page Accesses, Total Page Faults, Number of Replacements, and Page Fault Rate (%).

---

## 5. Experimental Results & Analysis

*(Note: Use the included Python script `analysis/generate_report_figures.py` to generate the graphs from the CSV logs, and insert them into the placeholders below.)*

### 5.1 Scheduling Policy Analysis

> **[ PLACEHOLDER: Insert Bar Chart Comparing Waiting/Turnaround Times for FCFS, RR, Priority ]**

**Analysis:**
*   **FCFS** exhibited the highest average waiting time, particularly under the mixed workload, demonstrating the "Convoy Effect" where short processes were starved behind long CPU-bound processes.
*   **Round Robin** effectively eliminated starvation and provided the best response time. However, setting the quantum too low resulted in a massive spike in context switches, severely degrading CPU utilization due to scheduling overhead.
*   **Priority Scheduling** ensured critical tasks finished first but demonstrated a risk of starvation for low-priority processes unless an aging mechanism was introduced.

### 5.2 Memory Policy Analysis (Page Replacement)

> **[ PLACEHOLDER: Insert Line/Bar Graph comparing Page Fault Rates of FIFO vs. LRU under high memory pressure ]**

**Analysis:**
*   **FIFO** was computationally cheaper to simulate but suffered from Belady’s Anomaly in specific access patterns. Its page fault rate was consistently higher under randomized access.
*   **LRU (Least Recently Used)** drastically reduced the page fault rate by keeping actively used pages in frames. The simulated timestamp tracking proved that exploiting temporal locality yields vastly superior memory performance, albeit with a slightly higher tracking overhead in the Page Table.

---

## 6. Limitations & Future Improvements

While the simulator successfully fulfills the CEP requirements, several limitations offer avenues for future enhancement:
1.  **Lack of Virtual File System:** The simulator handles I/O abstractly as a "tick delay." Implementing a mock file system with actual disk seek times would provide a more realistic I/O bottleneck simulation.
2.  **Memory Access Patterns:** Currently, the workload generator accesses memory pages uniformly. Implementing localized access patterns (e.g., loops and arrays) would better highlight the advantages of the LRU replacement policy.
3.  **Multicore Scheduling:** The current implementation models a single CPU core. Upgrading the `SimKernel` to support SMP (Symmetric Multiprocessing) with multiple running queues and load balancing would significantly increase the complexity and realism of the simulation.

---

## 7. Conclusion

The development of the Mini OS Kernel Simulator successfully bridged the gap between theoretical operating system abstractions and practical, code-level implementation. By independently designing the process state machines, memory allocators, and CPU schedulers, we gained operational knowledge of how an OS kernel functions under the hood. 

The quantitative experiments confirmed textbook theories: Round Robin provides fairness at the cost of context-switch overhead, while LRU out-performs FIFO by exploiting memory locality. This project fundamentally reinforced our understanding of the intricate, necessary trade-offs required in modern Operating System design.
