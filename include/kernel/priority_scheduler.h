#pragma once
#include "scheduler.h"
#include <vector>
#include <algorithm>

// Priority Scheduler
// Supports both preemptive and non-preemptive modes
// Lower priority number = higher priority (e.g., priority 1 runs before priority 5)
class PriorityScheduler : public Scheduler {
private:
    std::vector<PCB*> ready_queue;
    bool preemptive;

    void sortQueue() {
        std::sort(ready_queue.begin(), ready_queue.end(),
            [](const PCB* a, const PCB* b) {
                if (a->priority != b->priority)
                    return a->priority < b->priority; // lower number = higher priority
                return a->arrival_time < b->arrival_time; // FCFS tiebreak
            });
    }

public:
    explicit PriorityScheduler(bool preempt = true) : preemptive(preempt) {}

    void addProcess(PCB* process) override {
        ready_queue.push_back(process);
        sortQueue();
    }

    PCB* getNext() override {
        if (ready_queue.empty()) return nullptr;
        PCB* next = ready_queue.front();
        ready_queue.erase(ready_queue.begin());
        return next;
    }

    PCB* peekNext() const override {
        if (ready_queue.empty()) return nullptr;
        return ready_queue.front();
    }

    bool isEmpty() const override { return ready_queue.empty(); }
    int size() const override { return (int)ready_queue.size(); }
    bool isPreemptive() const override { return preemptive; }

    // In preemptive mode, preempt if a higher-priority process is waiting
    bool shouldPreempt(PCB* current, int ticks_on_cpu) override {
        if (!preemptive || ready_queue.empty()) return false;
        return ready_queue.front()->priority < current->priority;
    }

    std::string name() const override {
        return preemptive ? "Priority(Preemptive)" : "Priority(NonPreemptive)";
    }
};
