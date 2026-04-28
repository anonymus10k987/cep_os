#pragma once
#include "scheduler.h"
#include <queue>

// First-Come-First-Served Scheduler
// Non-preemptive: once a process starts, it runs until burst completes or I/O
class FCFSScheduler : public Scheduler {
private:
    std::queue<PCB*> ready_queue;

public:
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
    bool isPreemptive() const override { return false; }
    std::string name() const override { return "FCFS"; }
};
