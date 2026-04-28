 /*Mini OS Kernel Simulator
 * CS-330: Operating Systems - Semester Design Project (CEP)
 *
 * Simulates fundamental OS functionalities:
 *   - Process & Thread Management
 *   - CPU Scheduling (FCFS, Round Robin, Priority)
 *   - Process Synchronization (Mutex, Semaphore)
 *   - Memory Management (Paging with FIFO/LRU replacement)
 *
 * Usage:
 *   os_simulator [--config <file>] [--scheduler fcfs|rr|priority]
 *                [--quantum <n>] [--replacement fifo|lru]
 *                [--processes <n>] [--frames <n>]
 *                [--mode scheduling|sync|memory|full]
 */

#include "../include/sim_kernel.h"
#include "../include/kernel/fcfs_scheduler.h"
#include "../include/kernel/round_robin_scheduler.h"
#include "../include/kernel/priority_scheduler.h"
#include "../include/kernel/sync/mutex.h"
#include "../include/kernel/sync/semaphore.h"
#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#ifdef _WIN32
  #include <windows.h>
#endif

namespace fs = std::filesystem;

// ============================================================
// Demo Mode: Scheduling Comparison
// Runs the same workload under all 3 schedulers for comparison
// ============================================================
void runSchedulingComparison(const SimConfig& base_config) {
    std::cout << " SCHEDULING ALGORITHM COMPARISON " << std::endl;
    std::cout << " =============================== " << std::endl;

    std::string schedulers[] = {"fcfs", "rr", "priority"};

    for (auto& sched_name : schedulers) {
        SimConfig cfg = base_config;
        cfg.scheduler = sched_name;

        // Use a separate log dir for each scheduler
        cfg.log_dir = "output/logs/" + sched_name;
        fs::create_directories(cfg.log_dir);

        Scheduler* sched = nullptr;
        if (sched_name == "fcfs") {
            sched = new FCFSScheduler();
        } else if (sched_name == "rr") {
            sched = new RoundRobinScheduler(cfg.time_quantum);
        } else {
            sched = new PriorityScheduler(cfg.preemptive_priority);
        }

        SimKernel kernel(cfg, sched);
        kernel.init();
        kernel.run();
        kernel.printResults();

        delete sched;
    }
}

// ============================================================
// Demo Mode: Synchronization Demonstration
// Shows mutex/semaphore behavior including race conditions
// ============================================================
void runSyncDemo() {
    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║      SYNCHRONIZATION DEMONSTRATION               ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n";

    // ---- Part 1: Race Condition (NO synchronization) ----
    std::cout << "\n--- Part 1: Race Condition (WITHOUT sync) ---\n";
    {
        int shared_counter = 0;
        const int iterations = 100000;
        std::vector<std::thread> threads;

        for (int t = 0; t < 4; t++) {
            threads.emplace_back([&shared_counter, iterations]() {
                for (int i = 0; i < iterations; i++) {
                    shared_counter++;  // NOT thread-safe!
                }
            });
        }
        for (auto& th : threads) th.join();

        std::cout << "Expected counter value: " << (4 * iterations) << std::endl;
        std::cout << "Actual counter value:   " << shared_counter << std::endl;
        if (shared_counter != 4 * iterations) {
            std::cout << ">>> RACE CONDITION DETECTED! Lost "
                      << (4 * iterations - shared_counter) << " increments\n";
        }
    }

    // ---- Part 2: With Mutex (synchronized) ----
    std::cout << "\n--- Part 2: With Mutex (synchronized) ---\n";
    {
        int shared_counter = 0;
        const int iterations = 100000;
        std::mutex mtx;
        std::vector<std::thread> threads;

        for (int t = 0; t < 4; t++) {
            threads.emplace_back([&shared_counter, &mtx, iterations]() {
                for (int i = 0; i < iterations; i++) {
                    std::lock_guard<std::mutex> lock(mtx);
                    shared_counter++;  // Thread-safe with mutex
                }
            });
        }
        for (auto& th : threads) th.join();

        std::cout << "Expected counter value: " << (4 * iterations) << std::endl;
        std::cout << "Actual counter value:   " << shared_counter << std::endl;
        std::cout << ">>> Result: " << (shared_counter == 4 * iterations ? "CORRECT - No data loss!" : "ERROR") << "\n";
    }

    // ---- Part 3: Producer-Consumer with Semaphore Simulation ----
    std::cout << "\n--- Part 3: Producer-Consumer (Semaphore Simulation) ---\n";
    {
        const int BUFFER_SIZE = 5;
        const int NUM_ITEMS = 15;
        std::vector<int> buffer;
        std::mutex buffer_mtx;
        std::mutex print_mtx;

        // Using counting semaphores via condition variables
        std::mutex sem_mtx;
        std::condition_variable not_full;
        std::condition_variable not_empty;
        int item_count = 0;

        std::atomic<bool> done_producing{false};

        // Producer thread
        std::thread producer([&]() {
            for (int i = 1; i <= NUM_ITEMS; i++) {
                // Wait if buffer is full
                {
                    std::unique_lock<std::mutex> lock(sem_mtx);
                    not_full.wait(lock, [&]() { return item_count < BUFFER_SIZE; });
                }

                {
                    std::lock_guard<std::mutex> lock(buffer_mtx);
                    buffer.push_back(i);
                    item_count++;
                }

                {
                    std::lock_guard<std::mutex> lock(print_mtx);
                    std::cout << "  [Producer] Produced item " << i
                              << " (buffer size: " << item_count << "/" << BUFFER_SIZE << ")\n";
                }

                not_empty.notify_one();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            done_producing = true;
            not_empty.notify_all();
        });

        // Consumer thread
        std::thread consumer([&]() {
            int consumed = 0;
            while (consumed < NUM_ITEMS) {
                int item;
                {
                    std::unique_lock<std::mutex> lock(sem_mtx);
                    not_empty.wait(lock, [&]() {
                        return item_count > 0 || done_producing.load();
                    });
                    if (item_count == 0 && done_producing.load()) break;
                }

                {
                    std::lock_guard<std::mutex> lock(buffer_mtx);
                    if (buffer.empty()) continue;
                    item = buffer.front();
                    buffer.erase(buffer.begin());
                    item_count--;
                }

                consumed++;

                {
                    std::lock_guard<std::mutex> lock(print_mtx);
                    std::cout << "  [Consumer] Consumed item " << item
                              << " (buffer size: " << item_count << "/" << BUFFER_SIZE << ")\n";
                }

                not_full.notify_one();
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            std::cout << "  [Consumer] Total items consumed: " << consumed << "\n";
        });

        producer.join();
        consumer.join();
        std::cout << ">>> Producer-Consumer completed successfully!\n";
    }

    // ---- Part 4: Simulated Mutex/Semaphore (kernel-level) ----
    std::cout << "\n--- Part 4: SimKernel Mutex/Semaphore Trace ---\n";
    {
        SimMutex mtx(0);
        SimSemaphore sem(0, 2); // binary semaphore with count=2

        std::cout << "  SimMutex test:\n";
        std::cout << "    TID1 lock: " << (mtx.lock(1) ? "acquired" : "blocked") << "\n";
        std::cout << "    TID2 lock: " << (mtx.lock(2) ? "acquired" : "blocked") << "\n";
        std::cout << "    TID3 lock: " << (mtx.lock(3) ? "acquired" : "blocked") << "\n";
        std::cout << "    Mutex owner: TID" << mtx.getOwner()
                  << ", waiters: " << mtx.getWaitQueueSize() << "\n";
        int woken = mtx.unlock(1);
        std::cout << "    TID1 unlock -> woke TID" << woken << "\n";
        woken = mtx.unlock(2);
        std::cout << "    TID2 unlock -> woke TID" << woken << "\n";

        std::cout << "\n  SimSemaphore test (count=2):\n";
        std::cout << "    TID1 wait: " << (sem.wait(1) ? "proceed" : "blocked") << " (count=" << sem.getCount() << ")\n";
        std::cout << "    TID2 wait: " << (sem.wait(2) ? "proceed" : "blocked") << " (count=" << sem.getCount() << ")\n";
        std::cout << "    TID3 wait: " << (sem.wait(3) ? "proceed" : "blocked") << " (count=" << sem.getCount() << ")\n";
        std::cout << "    Waiters: " << sem.getWaitQueueSize() << "\n";
        woken = sem.signal();
        std::cout << "    Signal -> woke TID" << woken << " (count=" << sem.getCount() << ")\n";
        woken = sem.signal();
        std::cout << "    Signal -> " << (woken == -1 ? "incremented count" : "woke TID" + std::to_string(woken))
                  << " (count=" << sem.getCount() << ")\n";
    }
}

// ============================================================
// Demo Mode: Memory/Paging Demonstration
// Shows page fault handling and replacement policy comparison
// ============================================================
void runMemoryDemo(const SimConfig& base_config) {
    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║      MEMORY MANAGEMENT DEMONSTRATION             ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n";

    // Test with a known page reference string
    // Using the classic textbook example: 7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
    std::vector<int> reference_string = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
    int num_frames = 3; // small frame count to force replacements

    std::string policies[] = {"fifo", "lru"};

    for (auto& policy_name : policies) {
        std::cout << "\n--- " << policy_name << " Page Replacement ---\n";
        std::cout << "Reference String: ";
        for (int r : reference_string) std::cout << r << " ";
        std::cout << "\nFrames: " << num_frames << "\n\n";

        ReplacementPolicy* policy;
        if (policy_name == "fifo") {
            policy = new FIFOReplacementPolicy();
        } else {
            policy = new LRUReplacementPolicy();
        }

        Logger log(true, false); // console only
        MemoryManager mm(num_frames, policy, &log);

        // Create a process with 8 virtual pages (pages 0-7)
        int pid = 1;
        mm.initProcess(pid, 8);

        int faults = 0;
        for (size_t i = 0; i < reference_string.size(); i++) {
            int page = reference_string[i];
            int old_faults = mm.getTotalPageFaults();
            mm.accessPage(pid, page, (int)i);
            if (mm.getTotalPageFaults() > old_faults) faults++;
        }

        std::cout << "\n  Total Page Faults: " << faults
                  << " / " << reference_string.size() << " accesses\n";
        std::cout << "  Page Fault Rate: " << std::fixed << std::setprecision(1)
                  << (100.0 * faults / reference_string.size()) << "%\n";

        // Show final page table
        auto* pt = mm.getPageTable(pid);
        if (pt) pt->print();

        delete policy;
    }
}

// ============================================================
// Parse command-line arguments
// ============================================================
SimConfig parseArgs(int argc, char* argv[]) {
    SimConfig config;
    std::string mode = "full";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            config.loadFromFile(argv[++i]);
        } else if (arg == "--scheduler" && i + 1 < argc) {
            config.scheduler = argv[++i];
        } else if (arg == "--quantum" && i + 1 < argc) {
            config.time_quantum = std::stoi(argv[++i]);
        } else if (arg == "--replacement" && i + 1 < argc) {
            config.replacement_policy = argv[++i];
        } else if (arg == "--processes" && i + 1 < argc) {
            config.num_processes = std::stoi(argv[++i]);
        } else if (arg == "--frames" && i + 1 < argc) {
            config.total_physical_frames = std::stoi(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            config.random_seed = std::stoi(argv[++i]);
        } else if (arg == "--quiet") {
            config.console_logging = false;
        } else if (arg == "--interactive") {
            config.interactive_mode = true;
            config.console_logging = false;
        } else if (arg == "--help") {
            std::cout << "Mini OS Kernel Simulator\n"
                      << "Usage: os_simulator [options]\n\n"
                      << "Options:\n"
                      << "  --config <file>         Load config from file\n"
                      << "  --scheduler <algo>      fcfs, rr, priority\n"
                      << "  --quantum <n>           Time quantum for RR\n"
                      << "  --replacement <policy>  fifo, lru\n"
                      << "  --processes <n>         Number of processes\n"
                      << "  --frames <n>            Physical memory frames\n"
                      << "  --seed <n>              Random seed\n"
                      << "  --quiet                 Suppress tick-by-tick logging\n"
                      << "  --interactive           Run with visual Terminal UI\n"
                      << "  --mode <mode>           scheduling, sync, memory, full, compare\n"
                      << "  --help                  Show this help\n";
            exit(0);
        } else if (arg == "--mode" && i + 1 < argc) {
            mode = argv[++i];
        }
    }

    // Store mode in a hacky way via the config (we'll read it back in main)
    // Actually, let's just return it separately
    return config;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Switch terminal to UTF-8 so box-drawing characters render correctly
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // Enable ANSI escape codes (colours, cursor movement, clear-screen)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif

    std::cout << R"(
    ╔═══════════════════════════════════════════════════════╗
    ║              Mini OS Kernel Simulator                 ║
    ║         CS-330: Operating Systems - CEP               ║
    ╚═══════════════════════════════════════════════════════╝
    )" << std::endl;

    SimConfig config = parseArgs(argc, argv);

    // Determine mode
    std::string mode = "full";
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--mode" && i + 1 < argc) {
            mode = argv[i + 1];
        }
    }

    // Create output directories
    fs::create_directories("output/logs");
    fs::create_directories("output/logs/fcfs");
    fs::create_directories("output/logs/rr");
    fs::create_directories("output/logs/priority");

    if (mode == "compare" || mode == "full") {
        runSchedulingComparison(config);
    }

    if (mode == "sync" || mode == "full") {
        runSyncDemo();
    }

    if (mode == "memory" || mode == "full") {
        runMemoryDemo(config);
    }

    if (mode == "scheduling") {
        // Single scheduler run
        Scheduler* sched = nullptr;
        if (config.scheduler == "fcfs") {
            sched = new FCFSScheduler();
        } else if (config.scheduler == "rr") {
            sched = new RoundRobinScheduler(config.time_quantum);
        } else {
            sched = new PriorityScheduler(config.preemptive_priority);
        }

        fs::create_directories(config.log_dir);
        SimKernel kernel(config, sched);
        kernel.init();
        kernel.run();
        kernel.printResults();
        delete sched;
    }

    std::cout << "\n=== All demonstrations complete! ===\n";
    return 0;
}
