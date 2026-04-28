#pragma once
#include <queue>
#include <string>

// Simulated Counting Semaphore — models OS-level semaphore with P/V operations
class SimSemaphore {
private:
    int id;
    int count;                        // Current value
    int initial_count;                // For reset/reporting
    std::queue<int> wait_queue;       // TIDs blocked on wait()

public:
    explicit SimSemaphore(int _id = 0, int initial = 1)
        : id(_id), count(initial), initial_count(initial) {}

    // P operation (wait/down) — decrement or block
    // Returns true if thread can proceed, false if blocked
    bool wait(int tid) {
        if (count > 0) {
            count--;
            return true;  // proceeds
        } else {
            wait_queue.push(tid);
            return false; // blocked
        }
    }

    // V operation (signal/up) — increment or wake a waiting thread
    // Returns TID of woken thread, or -1 if no one waiting
    int signal() {
        if (wait_queue.empty()) {
            count++;
            return -1; // just increment
        } else {
            // Wake next waiter (don't increment — transfer permission)
            int next = wait_queue.front();
            wait_queue.pop();
            return next; // wake this thread
        }
    }

    int getId() const { return id; }
    int getCount() const { return count; }
    int getWaitQueueSize() const { return (int)wait_queue.size(); }
};
