#pragma once
#include <string>
#include <vector>
#include <iostream>

// Process states matching the standard OS process lifecycle
enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
};

// Convert state to string for logging
inline std::string stateToString(ProcessState s) {
    switch (s) {
        case ProcessState::NEW:        return "NEW";
        case ProcessState::READY:      return "READY";
        case ProcessState::RUNNING:    return "RUNNING";
        case ProcessState::WAITING:    return "WAITING";
        case ProcessState::TERMINATED: return "TERMINATED";
        default:                       return "UNKNOWN";
    }
}

// Process Control Block — the core kernel data structure for process management
struct PCB {
    int pid;                        // Unique process identifier
    std::string name;               // Human-readable name
    ProcessState state;             // Current process state

    int priority;                   // Scheduling priority (lower = higher priority)
    int arrival_time;               // Tick when process arrives

    // CPU burst modeling
    int total_cpu_burst;            // Total CPU time needed
    int remaining_burst;            // Remaining CPU time
    int io_burst_time;              // Time spent in I/O (simulated wait)
    int io_remaining;               // Remaining I/O wait time

    // Memory requirements (in pages)
    int memory_pages_required;
    std::vector<int> allocated_pages; // Page numbers allocated to this process

    // Scheduling metrics
    int waiting_time;               // Total time spent in READY queue
    int turnaround_time;            // Completion - Arrival
    int response_time;              // First run - Arrival

    // Timestamps
    int start_time;                 // First time this process ran
    int completion_time;            // When the process terminated

    // Thread tracking
    std::vector<int> thread_ids;    // TIDs belonging to this process

    // Constructor with defaults
    PCB()
        : pid(-1), name(""), state(ProcessState::NEW),
          priority(0), arrival_time(0),
          total_cpu_burst(0), remaining_burst(0),
          io_burst_time(0), io_remaining(0),
          memory_pages_required(0),
          waiting_time(0), turnaround_time(0), response_time(-1),
          start_time(-1), completion_time(-1) {}

    PCB(int _pid, const std::string& _name, int _priority,
        int _arrival, int _cpu_burst, int _io_burst, int _mem_pages)
        : pid(_pid), name(_name), state(ProcessState::NEW),
          priority(_priority), arrival_time(_arrival),
          total_cpu_burst(_cpu_burst), remaining_burst(_cpu_burst),
          io_burst_time(_io_burst), io_remaining(0),
          memory_pages_required(_mem_pages),
          waiting_time(0), turnaround_time(0), response_time(-1),
          start_time(-1), completion_time(-1) {}

    // Check if process has completed all CPU work
    bool isComplete() const { return remaining_burst <= 0; }

    // Print summary
    void print() const {
        std::cout << "PID=" << pid << " [" << name << "] State=" << stateToString(state)
                  << " Priority=" << priority << " Burst=" << remaining_burst
                  << "/" << total_cpu_burst << " Arrival=" << arrival_time << std::endl;
    }
};
