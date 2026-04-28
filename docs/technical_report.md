# CS-330: Operating System - Semester Design Project (CEP)
## Technical Report

**Instructor:** AP Mobeena Shahzad
**Course/Semester:** BESE 30 (A, B, C) - Spring 2026

---

### 1. Introduction

The objective of this Complex Engineering Problem (CEP) was to design and implement a **Mini Operating System Kernel Simulator** in user-space. This simulator effectively models fundamental attributes of modern OS kernels, including Process and Thread Management, CPU Scheduling, Process Synchronization, and Memory Management via Paging. Through simulating various dynamic workloads, the system captures trade-offs of performance and fairness under real constraints.

### 2. Design Methodology

The system is constructed with C++ (C++20 standards) integrating several loosely coupled components. It provides a cycle-accurate tick-based emulation loop for determinism and clear log tracing.

#### Architecture Components:
- **Process Manager:** Preserves `PCB` (Process Control Block) and `TCB` (Thread Control Block) elements, routing transitions among the *New*, *Ready*, *Running*, *Waiting*, and *Terminated* states.
- **CPU Scheduling:** Employs an interface abstracting scheduling behavior. Three algorithms were implemented: 
  - *First-Come-First-Serve (FCFS)*
  - *Round Robin (RR)*
  - *Preemptive Priority Scheduling*
- **Process Synchronization:** Features kernel space constructs: `SimMutex`, and `SimSemaphore` handling queues and lock ownership explicitly mapped to PIDs/TIDs. Uses `std::thread`, `std::mutex`, and `std::condition_variable` inside the simulation's native execution space to demonstrate real OS process synchronization.
- **Memory Management (Paging):** Uses a translation from Virtual Addresses via a `PageTable` array mapped to a simulated physical `FrameAllocator`. Memory bottlenecks and hit-ratios are handled dynamically via two distinct replacement strategies:
  - *First-In-First-Out (FIFO)*
  - *Least-Recently-Used (LRU)*
- **Data & Evaluation Pipeline:** A JSON/CSV `Logger` system records real-time logs that are later processed via Python scripts ensuring structured evaluation.

### 3. Implementation Details

We chose standard **C++** due to its explicit capacity to bind threads natively to the host OS allowing true demonstration of critical concurrency. 

#### System APIs Implemented:
The simulator implements basic syscall abstractions executed over time "ticks".
- `create_process(name, priority, burst, io, pages)`
- `setReady()`, `setWaiting()`, `setTerminated()`
- Synchronization primitives: `mutex_lock()`, `mutex_unlock()`, `sem_wait()`, `sem_signal()`
- Memory Requests: `accessPage(pid, pageNumber, currentTick, isWrite=false)`

### 4. Experimental Results and Analysis

Workloads simulated consisted of 10-15 independent processes with randomized attributes split into *CPU Bound*, *I/O Bound*, and *Mixed Data Loads*.

#### 4.1 CPU Scheduling

By exposing the identical Mixed-Load environment across our three schedulers:

1. **FCFS:** Served linearly. Yielded the highest max-turnaround-time but completely avoided CPU starvation. CPU Utilization proved lowest among dense I/O bursts due to waiting gaps.
2. **Round Robin (Quantum = 4):** Fostered responsive multiplexing reducing standard deviation in Response Times drastically. Frequent preemptions (measured via Context Switching counters) slightly ate into efficiency.
3. **Priority (Preemptive):** Maximally lowered wait times for designated high-priority tasks successfully. Low priority tasks saw longer turnaround times confirming adherence strictly to rule sets.

#### 4.2 Page Replacement (FIFO vs LRU)

On identical synthetic textbook reference strings (7, 0, 1, 2, 0, 3, 0...):
- **FIFO Strategy** aggressively evicted loaded memory blindly leading to higher absolute counts of page faults (e.g., Belady's Anomaly risk).
- **LRU Strategy** tracking historical references proved distinctly optimal consistently registering fewer faults relative to FIFO for our memory footprint configs.

#### 4.3 Synchronization Correctness

Demonstrating classic race conditions dynamically without synchronization primitives generated lost-updates predictably (divergence from mathematically expected sequence outputs). Incorporating our C++-mapped `SimMutex` securely restricted Critical Section access producing zero data losses indicating robust state enforcement across concurrency bounds.

### 5. Limitations & Future Improvements

**Current Limitations:**
- Emulates single-core CPU mapping only.
- Fixed logical memory scaling limits dynamic memory expansions during runtime (heaps).

**Enhancements:**
- Extrapolating `SimKernel` to account for Multi-Core affinity arrays.
- Moving to "Event-Driven" jumps over tick-counting architecture for extensive simulation efficiency when large delays emerge.

### 6. Conclusion 

This system validates every target of the Complex Engineering Activity accurately modeling true CPU lifecycles dynamically integrating synchronized concurrency constraints layered with virtual-physical abstraction and memory evaluations.
