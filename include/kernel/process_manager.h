#pragma once
#include "pcb.h"
#include "tcb.h"
#include "../utils/logger.h"
#include <vector>
#include <map>
#include <memory>
#include <algorithm>

class ProcessManager {
private:
    std::vector<PCB> processes;
    std::vector<TCB> threads;
    int next_pid = 1;
    int next_tid = 1;
    Logger* logger;

public:
    ProcessManager(Logger* log = nullptr) : logger(log) {}

    // Create a new process and return its PID
    int createProcess(const std::string& name, int priority,
                      int arrival_time, int cpu_burst,
                      int io_burst, int memory_pages) {
        int pid = next_pid++;
        PCB pcb(pid, name, priority, arrival_time, cpu_burst, io_burst, memory_pages);
        processes.push_back(pcb);

        if (logger) {
            logger->log(arrival_time, LogCategory::PROC,
                "Process created: " + name + " (PID=" + std::to_string(pid) +
                ", burst=" + std::to_string(cpu_burst) +
                ", priority=" + std::to_string(priority) +
                ", pages=" + std::to_string(memory_pages) + ")");
        }
        return pid;
    }

    // Create a thread under a process
    int createThread(int parent_pid, const std::string& name,
                     int priority, int burst) {
        int tid = next_tid++;
        TCB tcb(tid, parent_pid, name, priority, burst);
        threads.push_back(tcb);

        // Link to parent
        PCB* parent = getProcess(parent_pid);
        if (parent) {
            parent->thread_ids.push_back(tid);
        }

        if (logger) {
            logger->log(0, LogCategory::PROC,
                "Thread created: " + name + " (TID=" + std::to_string(tid) +
                ", parent PID=" + std::to_string(parent_pid) + ")");
        }
        return tid;
    }

    // State transitions
    void setReady(int pid, int tick) {
        PCB* p = getProcess(pid);
        if (!p) return;
        ProcessState old = p->state;
        p->state = ProcessState::READY;
        if (logger) {
            logger->log(tick, LogCategory::PROC,
                p->name + " (PID=" + std::to_string(pid) + ") " +
                stateToString(old) + " -> READY");
        }
    }

    void setRunning(int pid, int tick) {
        PCB* p = getProcess(pid);
        if (!p) return;
        ProcessState old = p->state;
        p->state = ProcessState::RUNNING;
        if (p->start_time == -1) {
            p->start_time = tick;
            p->response_time = tick - p->arrival_time;
        }
        if (logger) {
            logger->log(tick, LogCategory::PROC,
                p->name + " (PID=" + std::to_string(pid) + ") " +
                stateToString(old) + " -> RUNNING");
        }
    }

    void setWaiting(int pid, int tick) {
        PCB* p = getProcess(pid);
        if (!p) return;
        ProcessState old = p->state;
        p->state = ProcessState::WAITING;
        if (logger) {
            logger->log(tick, LogCategory::PROC,
                p->name + " (PID=" + std::to_string(pid) + ") " +
                stateToString(old) + " -> WAITING");
        }
    }

    void setTerminated(int pid, int tick) {
        PCB* p = getProcess(pid);
        if (!p) return;
        ProcessState old = p->state;
        p->state = ProcessState::TERMINATED;
        p->completion_time = tick;
        p->turnaround_time = tick - p->arrival_time;
        if (logger) {
            logger->log(tick, LogCategory::PROC,
                p->name + " (PID=" + std::to_string(pid) + ") " +
                stateToString(old) + " -> TERMINATED" +
                " (turnaround=" + std::to_string(p->turnaround_time) + ")");
        }
    }

    // Get process by PID (mutable)
    PCB* getProcess(int pid) {
        for (auto& p : processes) {
            if (p.pid == pid) return &p;
        }
        return nullptr;
    }

    // Get thread by TID
    TCB* getThread(int tid) {
        for (auto& t : threads) {
            if (t.tid == tid) return &t;
        }
        return nullptr;
    }

    // Get all processes
    std::vector<PCB>& getAllProcesses() { return processes; }
    const std::vector<PCB>& getAllProcesses() const { return processes; }

    // Get processes in a given state
    std::vector<PCB*> getProcessesByState(ProcessState state) {
        std::vector<PCB*> result;
        for (auto& p : processes) {
            if (p.state == state) result.push_back(&p);
        }
        return result;
    }

    // Get arrived but not-yet-admitted processes at a given tick
    std::vector<PCB*> getNewArrivals(int tick) {
        std::vector<PCB*> result;
        for (auto& p : processes) {
            if (p.arrival_time == tick && p.state == ProcessState::NEW) {
                result.push_back(&p);
            }
        }
        return result;
    }

    // Check if all processes are terminated
    bool allTerminated() const {
        return std::all_of(processes.begin(), processes.end(),
            [](const PCB& p) { return p.state == ProcessState::TERMINATED; });
    }

    // Count active (non-terminated) processes
    int activeCount() const {
        return (int)std::count_if(processes.begin(), processes.end(),
            [](const PCB& p) { return p.state != ProcessState::TERMINATED; });
    }
};
