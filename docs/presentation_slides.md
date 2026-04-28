# CS-330 Semester Project: Mini OS Kernel Simulator Presentation

## Slide 1: Title Slide
**Title:** Mini Operating System Kernel Simulator
**Subtitle:** CS-330 Complex Engineering Problem
**Course:** BESE 30 - Spring 2026
**Presenter:** [Your Name / Team Members]

---

## Slide 2: Project Objectives
- **Goal:** Design a user-space kernel simulator reflecting core OS functionality.
- **Key Subsystems:**
  - Process & Thread Management
  - CPU Scheduling (FCFS, Round Robin, Priority)
  - Process Synchronization (Mutex, Semaphore)
  - Memory Management (Paging, FIFO/LRU)
- **Methodology:** C++ implementation natively binding threads to OS for true concurrency.

---

## Slide 3: System Architecture
- *[Insert Architecture Diagram]*
- **SimKernel Core:** Central tick-based simulation loop.
- **Process Manager:** State transitions (PCB/TCB).
- **Subsystem Interfaces:** Memory Manager matching Virtual Pages to Physical Frames, Synchronizers restricting critical sections.

---

## Slide 4: CPU Scheduling Analysis
- *[Insert Scheduling Comparison Bar Chart & Gantt Chart]*
- **FCFS:** High turnaround times, standard linear service without starvation.
- **Round Robin:** Preemptive time-slicing (Quantum = 4), lowest response variance.
- **Priority:** Rapid execution of high-priority threads (lowest priority integers).

---

## Slide 5: Memory Management & Paging
- *[Insert Memory Fault Rate Chart]*
- **System Spec:** Simulated physical frames mapping to virtual process spaces. Fixed size.
- **Replacement Comparisons:** 
  - **FIFO:** Evicts oldest loaded frame. Suffers more total faults across complex I/O workloads.
  - **LRU:** Evicts least recently accessed frame. Highly performant keeping current working sets resident.

---

## Slide 6: Process Synchronization
- *[Insert Output snippet showing race condition vs mutex]*
- **Validation:** 
  - Without Mutex: Expected 400,000 updates natively diverged (lost updates tracked).
  - With `SimMutex`: Exactly 400,000 successful updates blocking overlapping writes efficiently.

---

## Slide 7: Conclusion
- **Achievements:** Met all rubric objectives (Depth of knowledge, experimental workload analysis).
- **Future Scope:** Expanding the simulator for multi-core tracking and dynamic heap memory mappings.
- **Questions?**
