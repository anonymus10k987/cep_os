#pragma once
#include <queue>
#include <string>

// Simulated Mutex — models OS-level mutex with ownership tracking
// This is a simulation-level mutex (not std::mutex) — it tracks which thread
// holds the lock and which threads are blocked waiting.
class SimMutex {
private:
    int id;
    bool locked;
    int owner_tid;                    // TID of current holder (-1 if free)
    std::queue<int> wait_queue;       // TIDs waiting to acquire

public:
    explicit SimMutex(int _id = 0) : id(_id), locked(false), owner_tid(-1) {}

    // Attempt to acquire the mutex
    // Returns true if acquired, false if blocked (caller added to wait queue)
    bool lock(int tid) {
        if (!locked) {
            locked = true;
            owner_tid = tid;
            return true;  // acquired
        } else {
            wait_queue.push(tid);
            return false; // blocked
        }
    }

    // Release the mutex
    // Returns TID of next thread to wake up, or -1 if no one waiting
    int unlock(int tid) {
        if (owner_tid != tid) return -1; // only owner can unlock

        if (wait_queue.empty()) {
            locked = false;
            owner_tid = -1;
            return -1; // no waiter
        } else {
            // Hand lock to next waiter
            int next = wait_queue.front();
            wait_queue.pop();
            owner_tid = next;
            return next; // wake this thread
        }
    }

    // Try to acquire (non-blocking)
    bool tryLock(int tid) {
        if (!locked) {
            locked = true;
            owner_tid = tid;
            return true;
        }
        return false;
    }

    int getId() const { return id; }
    bool isLocked() const { return locked; }
    int getOwner() const { return owner_tid; }
    int getWaitQueueSize() const { return (int)wait_queue.size(); }
};
