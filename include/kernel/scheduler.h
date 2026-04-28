#pragma once
#include "pcb.h"
#include <string>

// Abstract scheduler interface — all scheduling algorithms implement this
class Scheduler {
public:
    virtual ~Scheduler() = default;

    // Add a process to the ready queue
    virtual void addProcess(PCB* process) = 0;

    // Remove and return the next process to run (nullptr if queue empty)
    virtual PCB* getNext() = 0;

    // Peek at next without removing
    virtual PCB* peekNext() const = 0;

    // Check if the ready queue is empty
    virtual bool isEmpty() const = 0;

    // Return the number of processes in the ready queue
    virtual int size() const = 0;

    // Whether this scheduler is preemptive
    virtual bool isPreemptive() const = 0;

    // Whether current process should be preempted (for preemptive schedulers)
    // Called each tick with the currently running process
    virtual bool shouldPreempt(PCB* current, int ticks_on_cpu) { return false; }

    // Name of the scheduling algorithm
    virtual std::string name() const = 0;
};
