#pragma once
#include <string>

// Thread states (mirrors process states but at thread level)
enum class ThreadState {
    READY,
    RUNNING,
    WAITING,
    TERMINATED
};

inline std::string threadStateToString(ThreadState s) {
    switch (s) {
        case ThreadState::READY:      return "READY";
        case ThreadState::RUNNING:    return "RUNNING";
        case ThreadState::WAITING:    return "WAITING";
        case ThreadState::TERMINATED: return "TERMINATED";
        default:                      return "UNKNOWN";
    }
}

// Thread Control Block — lightweight execution unit within a process
struct TCB {
    int tid;                    // Unique thread identifier
    int parent_pid;             // Owning process PID
    std::string name;           // Thread name
    ThreadState state;          // Current thread state

    int priority;               // Thread-level priority
    int remaining_burst;        // Remaining work for this thread

    // Timestamps
    int start_time;
    int completion_time;

    TCB()
        : tid(-1), parent_pid(-1), name(""), state(ThreadState::READY),
          priority(0), remaining_burst(0),
          start_time(-1), completion_time(-1) {}

    TCB(int _tid, int _parent_pid, const std::string& _name,
        int _priority, int _burst)
        : tid(_tid), parent_pid(_parent_pid), name(_name),
          state(ThreadState::READY), priority(_priority),
          remaining_burst(_burst),
          start_time(-1), completion_time(-1) {}

    bool isComplete() const { return remaining_burst <= 0; }
};
