#pragma once
#include "scheduler.h"
#include <queue>

// Round Robin Scheduler
// Preemptive: each process gets a fixed time quantum, then is preempted
class RoundRobinScheduler : public Scheduler {
private:
    std::queue<PCB*> ready_queue;
    int time_quantum;

public:
    explicit RoundRobinScheduler(int quantum = 4) : time_quantum(quantum) {}

    void addProcess(PCB* process) override {
        ready_queue.push(process);
    }

    PCB* getNext() override {
        if (ready_queue.empty()) return nullptr;
        PCB* next = ready_queue.front();
        ready_queue.pop();
        return next;
    }

    PCB* peekNext() const override {
        if (ready_queue.empty()) return nullptr;
        return ready_queue.front();
    }

    bool isEmpty() const override { return ready_queue.empty(); }
    int size() const override { return (int)ready_queue.size(); }
    bool isPreemptive() const override { return true; }

    // Preempt when quantum expires
    bool shouldPreempt(PCB* current, int ticks_on_cpu) override {
        return ticks_on_cpu >= time_quantum;
    }

    int getQuantum() const { return time_quantum; }
    std::string name() const override { return "RoundRobin(q=" + std::to_string(time_quantum) + ")"; }
};
