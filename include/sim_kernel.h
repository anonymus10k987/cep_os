#pragma once
#include "kernel/process_manager.h"
#include "kernel/scheduler.h"
#include "kernel/memory/memory_manager.h"
#include "kernel/sync/mutex.h"
#include "kernel/sync/semaphore.h"
#include "utils/logger.h"
#include "utils/metrics.h"
#include "utils/config.h"
#include "workload/workload_generator.h"
#include <map>
#include <random>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#ifdef _WIN32
  #include <conio.h>
#endif

// The SimKernel integrates all OS subsystems and runs the main simulation loop
class SimKernel {
private:
    SimConfig config;
    Logger logger;
    ProcessManager process_manager;
    Scheduler* scheduler;
    MemoryManager* memory_manager;
    ReplacementPolicy* replacement_policy;
    WorkloadGenerator workload_gen;

    // Simulation state
    int current_tick = 0;
    PCB* running_process = nullptr;
    int ticks_on_cpu = 0;       // how long current process has been running
    int context_switches = 0;
    int idle_ticks = 0;

    // Random generator for memory access simulation
    std::mt19937 rng;

public:
    SimKernel(const SimConfig& cfg, Scheduler* sched)
        : config(cfg),
          logger(cfg.console_logging, cfg.file_logging, cfg.log_dir),
          process_manager(&logger),
          scheduler(sched),
          memory_manager(nullptr),
          replacement_policy(nullptr),
          workload_gen(cfg.random_seed),
          rng(cfg.random_seed + 1)
    {
        // Create replacement policy
        if (cfg.replacement_policy == "fifo") {
            replacement_policy = new FIFOReplacementPolicy();
        } else {
            replacement_policy = new LRUReplacementPolicy();
        }

        memory_manager = new MemoryManager(cfg.total_physical_frames,
                                           replacement_policy, &logger);
    }

    ~SimKernel() {
        delete memory_manager;
        delete replacement_policy;
    }

    // Initialize: generate workload and set up memory
    void init() {
        logger.init();

        logger.log(0, LogCategory::KERNEL, "=== SimKernel Initializing ===");
        logger.log(0, LogCategory::KERNEL, "Scheduler: " + scheduler->name());
        logger.log(0, LogCategory::KERNEL, "Replacement Policy: " + replacement_policy->name());

        config.print();

        // Generate workload
        workload_gen.generate(process_manager, config);

        // Initialize page tables for all processes
        for (auto& p : process_manager.getAllProcesses()) {
            memory_manager->initProcess(p.pid, p.memory_pages_required);
        }

        logger.log(0, LogCategory::KERNEL,
            "Created " + std::to_string(config.num_processes) + " processes");
    }

    // Main simulation loop — tick-based execution
    void run() {
        logger.log(0, LogCategory::KERNEL, "=== Simulation Starting ===");

        if (config.interactive_mode) {
            runInteractive();
        } else {
            int max_ticks = 10000;
            while (!process_manager.allTerminated() && current_tick < max_ticks) {
                tick();
                current_tick++;
            }
            if (current_tick >= max_ticks)
                logger.log(current_tick, LogCategory::KERNEL, "WARNING: Simulation hit max tick limit!");
        }

        logger.log(current_tick, LogCategory::KERNEL,
            "=== Simulation Complete at tick " + std::to_string(current_tick) + " ===");
    }

    // ----------------------------------------------------------------
    // Interactive TUI mode
    //   Keys (non-blocking stdin check):
    //     SPACE / p  = pause / resume
    //     +/-        = speed up / slow down
    //     q / Q      = quit
    // ----------------------------------------------------------------
    void runInteractive() {
        // milliseconds per tick (adjustable at runtime)
        int delay_ms   = 300;
        bool paused    = false;
        bool quit_flag = false;
        int  max_ticks = 10000;

        // Print banner and instructions once
        clearScreen();
        std::cout << ansi("1;36") << R"(
  ╔══════════════════════════════════════════════════════════════════╗
  ║        Mini OS Kernel Simulator  —  Interactive TUI Mode        ║
  ║  Keys:  SPACE=pause  +/-=speed  q=quit                         ║
  ╚══════════════════════════════════════════════════════════════════╝
)" << ansi("0") << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));



        while (!process_manager.allTerminated() &&
               current_tick < max_ticks &&
               !quit_flag)
        {
            // --- non-blocking keypress check ---
            checkKeypress(paused, quit_flag, delay_ms);
            if (quit_flag) break;

            if (!paused) {
                tick();
                printDashboard(delay_ms, paused);
                current_tick++;
            } else {
                // Keep redrawing so the PAUSED indicator shows
                printDashboard(delay_ms, paused);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }

        if (!quit_flag) {
            // Final frame
            printDashboard(delay_ms, false);
            std::cout << "\n" << ansi("1;32")
                      << "  === Simulation finished at tick " << current_tick
                      << " === Press ENTER to see results." << ansi("0") << "\n";
            std::cin.get();
        }
    }

    // Single simulation tick
    void tick() {
        // 1. Admit newly arrived processes
        admitNewProcesses();

        // 2. Handle I/O completions (unblock waiting processes)
        handleIOCompletions();

        // 3. Check preemption
        if (running_process && scheduler->isPreemptive()) {
            if (scheduler->shouldPreempt(running_process, ticks_on_cpu)) {
                logger.logScheduling(current_tick, "PREEMPTED",
                    running_process->pid, running_process->name,
                    scheduler->name(),
                    "after " + std::to_string(ticks_on_cpu) + " ticks");
                process_manager.setReady(running_process->pid, current_tick);
                scheduler->addProcess(running_process);
                running_process = nullptr;
                ticks_on_cpu = 0;
            }
        }

        // 4. If CPU is idle, pick next process
        if (!running_process && !scheduler->isEmpty()) {
            running_process = scheduler->getNext();
            if (running_process) {
                process_manager.setRunning(running_process->pid, current_tick);
                ticks_on_cpu = 0;
                context_switches++;

                logger.logScheduling(current_tick, "DISPATCHED",
                    running_process->pid, running_process->name,
                    scheduler->name(),
                    "burst_remaining=" + std::to_string(running_process->remaining_burst));
            }
        }

        // 5. Execute current process for one tick
        if (running_process) {
            executeTick();
        } else {
            idle_ticks++;
        }

        // 6. Increment waiting time for all READY processes
        for (auto& p : process_manager.getAllProcesses()) {
            if (p.state == ProcessState::READY) {
                p.waiting_time++;
            }
        }
    }

    // Admit processes that arrive at current tick
    void admitNewProcesses() {
        auto arrivals = process_manager.getNewArrivals(current_tick);
        for (auto* p : arrivals) {
            process_manager.setReady(p->pid, current_tick);
            scheduler->addProcess(p);

            // Allocate initial pages (first page or two)
            int initial_pages = std::min(2, p->memory_pages_required);
            for (int pg = 0; pg < initial_pages; pg++) {
                memory_manager->accessPage(p->pid, pg, current_tick);
            }
        }
    }

    // Check if any waiting processes have completed I/O
    void handleIOCompletions() {
        auto waiting = process_manager.getProcessesByState(ProcessState::WAITING);
        for (auto* p : waiting) {
            p->io_remaining--;
            if (p->io_remaining <= 0) {
                process_manager.setReady(p->pid, current_tick);
                scheduler->addProcess(p);
                logger.log(current_tick, LogCategory::IO,
                    p->name + " (PID=" + std::to_string(p->pid) + ") I/O complete, moved to READY");
            }
        }
    }

    // Execute one tick of the running process
    void executeTick() {
        if (!running_process) return;

        running_process->remaining_burst--;
        ticks_on_cpu++;

        // Simulate memory access (random page access during execution)
        simulateMemoryAccess();

        // Check if burst complete
        if (running_process->remaining_burst <= 0) {
            // Check if process needs I/O
            if (running_process->io_burst_time > 0 && running_process->io_remaining <= 0) {
                // First I/O cycle
                running_process->io_remaining = running_process->io_burst_time;
                running_process->io_burst_time = 0; // consumed the I/O burst
                process_manager.setWaiting(running_process->pid, current_tick);
                logger.log(current_tick, LogCategory::IO,
                    running_process->name + " (PID=" + std::to_string(running_process->pid) +
                    ") starting I/O for " + std::to_string(running_process->io_remaining) + " ticks");
                running_process = nullptr;
                ticks_on_cpu = 0;
            } else {
                // Process complete
                process_manager.setTerminated(running_process->pid, current_tick);
                memory_manager->deallocateProcess(running_process->pid);
                logger.logScheduling(current_tick, "COMPLETED",
                    running_process->pid, running_process->name,
                    scheduler->name(), "");
                running_process = nullptr;
                ticks_on_cpu = 0;
            }
        }
    }

    // Simulate a memory page access for the running process
    void simulateMemoryAccess() {
        if (!running_process) return;

        // Access a random page within the process's virtual address space
        std::uniform_int_distribution<int> dist(0, running_process->memory_pages_required - 1);
        int page = dist(rng);
        bool is_write = (dist(rng) % 3 == 0); // ~33% writes

        memory_manager->accessPage(running_process->pid, page, current_tick, is_write);
    }

    // Print final results
    void printResults() {
        // Scheduling metrics
        auto metrics = MetricsCollector::computeSchedulingMetrics(
            scheduler->name(),
            process_manager.getAllProcesses(),
            current_tick,
            context_switches,
            idle_ticks
        );
        MetricsCollector::printSchedulingMetrics(metrics);

        // Memory metrics
        memory_manager->printStatus();

        // Export to files
        MetricsCollector::exportToJSON(metrics, config.log_dir + "/scheduling_metrics.json");

        MemoryMetrics mm;
        mm.replacement_policy = replacement_policy->name();
        mm.total_frames = config.total_physical_frames;
        mm.total_page_accesses = memory_manager->getTotalPageAccesses();
        mm.total_page_faults = memory_manager->getTotalPageFaults();
        mm.total_page_replacements = memory_manager->getTotalReplacements();
        mm.page_fault_rate = memory_manager->getPageFaultRate();
        mm.frame_utilization = memory_manager->getFrameUtilization();
        MetricsCollector::exportMemoryMetrics(mm, config.log_dir + "/memory_metrics.json");
    }

    // ----------------------------------------------------------------
    // Rich TUI Dashboard
    // ----------------------------------------------------------------
    void printDashboard(int delay_ms = 300, bool paused = false) {
        clearScreen();

        const int W = 68; // inner box width
        auto hline = [&]() {
            std::string s;
            for (int i = 0; i < W; i++) s += "\u2550";
            return s;
        };
        auto boxLine = [&](const std::string& content) {
            // Pad / truncate to W chars (ignoring ANSI escapes for width)
            std::string row = content;
            // approximate visible length (strip \033[...m)
            int vis = 0;
            bool esc = false;
            for (char c : row) {
                if (c == '\033') { esc = true; continue; }
                if (esc) { if (c == 'm') esc = false; continue; }
                vis++;
            }
            int pad = W - vis;
            if (pad > 0) row += std::string(pad, ' ');
            std::cout << "║" << row << "║\n";
        };

        // ── Header ──────────────────────────────────────────────────
        std::cout << "╔" << hline() << "╗\n";
        {
            std::string title = ansi("1;36") + "  Mini OS Kernel Simulator" + ansi("0");
            std::string right = "Tick: " + ansi("1;33") + padL(current_tick, 5) + ansi("0")
                              + "  Speed: " + ansi("1;32") + padL(delay_ms, 4) + "ms" + ansi("0")
                              + (paused ? ("  " + ansi("1;31") + "[PAUSED]" + ansi("0")) : "");
            // fixed visible widths: title≈26, right≈28
            std::string row = title + std::string(14, ' ') + right;
            boxLine(row);
        }
        {
            std::string row = "  Scheduler: " + ansi("1;35") + padR(scheduler->name(), 22) + ansi("0")
                            + "Policy: " + ansi("1;35") + config.replacement_policy
                            + " (" + std::to_string(config.total_physical_frames) + " frames)" + ansi("0");
            boxLine(row);
        }
        std::cout << "╠" << hline() << "╣\n";

        // ── CPU ─────────────────────────────────────────────────────
        boxLine(ansi("1;36") + "  ▶ CPU CORE" + ansi("0"));
        if (running_process) {
            PCB* rp = running_process;
            int total  = rp->total_cpu_burst;
            int remain = rp->remaining_burst;
            int done   = total - remain;
            // progress bar (40 chars)
            int BAR = 40;
            int filled = (total > 0) ? (done * BAR / total) : 0;
            std::string bar;
            for (int b = 0; b < filled; b++)        bar += ansi("32") + "\u2588" + ansi("0");
            for (int b = filled; b < BAR; b++)       bar += ansi("90") + "\u2591" + ansi("0");
            std::string row1 = "  " + ansi("1;32") + "RUNNING" + ansi("0")
                             + "  " + rp->name + " (PID " + std::to_string(rp->pid) + ")"
                             + "  prio=" + std::to_string(rp->priority);
            std::string row2 = "  [" + bar + "]  "
                             + padL(done,4) + "/" + padL(total,4)
                             + "  on-cpu=" + padL(ticks_on_cpu,3);
            boxLine(row1);
            boxLine(row2);
        } else {
            boxLine("  " + ansi("1;90") + "[ IDLE ]" + ansi("0") + "  no process dispatched");
            boxLine("");
        }
        std::cout << "╠" << hline() << "╣\n";

        // ── Process Table ────────────────────────────────────────────
        boxLine(ansi("1;36") + "  ▶ PROCESS TABLE" + ansi("0"));
        boxLine(ansi("90") + "  PID  Name       State       Burst  Wait   I/O   Pages" + ansi("0"));
        for (auto& p : process_manager.getAllProcesses()) {
            std::string stateColor;
            switch (p.state) {
                case ProcessState::RUNNING:    stateColor = "1;32"; break;
                case ProcessState::READY:      stateColor = "1;33"; break;
                case ProcessState::WAITING:    stateColor = "1;34"; break;
                case ProcessState::TERMINATED: stateColor = "90";   break;
                default:                       stateColor = "37";   break;
            }
            std::string stStr = padR(stateToString(p.state), 11);
            std::string ioStr = p.state == ProcessState::WAITING
                              ? padL(p.io_remaining, 4) : "   -";
            std::string row = "  " + padL(p.pid, 3) + "  "
                            + padR(p.name, 10) + " "
                            + ansi(stateColor) + stStr + ansi("0") + " "
                            + padL(p.remaining_burst, 5) + "  "
                            + padL(p.waiting_time,    4) + "  "
                            + ioStr + "  "
                            + padL(p.memory_pages_required, 4);
            boxLine(row);
        }
        std::cout << "╠" << hline() << "╣\n";

        // ── Ready Queue ──────────────────────────────────────────────
        {
            auto rq = process_manager.getProcessesByState(ProcessState::READY);
            std::string row = ansi("1;33") + "  ▶ READY QUEUE  " + ansi("0");
            if (rq.empty()) {
                row += ansi("90") + "(empty)" + ansi("0");
            } else {
                for (auto* p : rq)
                    row += "[" + ansi("1;33") + p->name + ansi("0") + "] ";
            }
            boxLine(row);
        }
        {
            auto wq = process_manager.getProcessesByState(ProcessState::WAITING);
            std::string row = ansi("1;34") + "  ▶ I/O WAITING  " + ansi("0");
            if (wq.empty()) {
                row += ansi("90") + "(empty)" + ansi("0");
            } else {
                for (auto* p : wq)
                    row += "[" + ansi("1;34") + p->name + ":" + std::to_string(p->io_remaining) + "t" + ansi("0") + "] ";
            }
            boxLine(row);
        }
        std::cout << "╠" << hline() << "╣\n";

        // ── Memory Frames ────────────────────────────────────────────
        boxLine(ansi("1;36") + "  ▶ PHYSICAL MEMORY  " + ansi("0")
              + std::to_string(memory_manager->getFrameAllocator().getAllocatedCount())
              + "/" + std::to_string(config.total_physical_frames) + " frames used");
        {
            // Build frame->owner map
            std::map<int, std::pair<int,int>> fo;
            for (auto& [pid, pt] : memory_manager->getAllPageTables())
                for (int pg = 0; pg < pt.getNumPages(); pg++) {
                    auto& e = pt.getEntry(pg);
                    if (e.valid) fo[e.frame_number] = {pid, pg};
                }

            // Process-colour cycling (pid 1→6 colours)
            static const char* pcolors[] = {"32","33","34","35","36","31","92","93"};
            int total_frames = config.total_physical_frames;
            const int COLS = 16;
            std::string row;
            int vis = 0;
            for (int i = 0; i < total_frames; i++) {
                if (i > 0 && i % COLS == 0) {
                    // pad and flush
                    boxLine("  " + row);
                    row.clear(); vis = 0;
                }
                if (fo.count(i)) {
                    auto [pid, pg] = fo[i];
                    const char* col = pcolors[(pid-1) % 8];
                    std::string cell = std::string("P") + std::to_string(pid) + ":" + (pg<10?"0":"") + std::to_string(pg);
                    row += ansi(col) + "█" + ansi("0");
                    vis += 1;
                } else {
                    row += ansi("90") + "░" + ansi("0");
                    vis += 1;
                }
                row += " ";
                vis += 1;
            }
            if (!row.empty()) boxLine("  " + row);
        }
        std::cout << "╠" << hline() << "╣\n";

        // ── Live Metrics ─────────────────────────────────────────────
        boxLine(ansi("1;36") + "  ▶ LIVE METRICS" + ansi("0"));
        {
            double fr = memory_manager->getPageFaultRate() * 100.0;
            int faults = memory_manager->getTotalPageFaults();
            int accesses = memory_manager->getTotalPageAccesses();
            double util = memory_manager->getFrameAllocator().getUtilization();

            // Active / done counts
            int done = 0, active = 0;
            for (auto& p : process_manager.getAllProcesses())
                (p.state == ProcessState::TERMINATED ? done : active)++;

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1);
            oss << "  Ctx-switches: " << padL(context_switches,4)
                << "   Idle ticks: " << padL(idle_ticks,4)
                << "   CPU util: " << ((current_tick > 0)
                    ? std::to_string((int)(100.0*(current_tick-idle_ticks)/current_tick)) : "0")
                << "%";
            boxLine(oss.str());

            std::ostringstream oss2;
            oss2 << std::fixed << std::setprecision(1);
            oss2 << "  Page faults: " << padL(faults,4) << "/" << padL(accesses,5)
                 << "  (" << fr << "%)   Mem util: "
                 << std::to_string((int)util) << "%"
                 << "   Done: " << done << "/" << (done+active);
            boxLine(oss2.str());
        }
        std::cout << "╚" << hline() << "╝\n";
        std::cout << ansi("90") + "  [SPACE]=pause  [+/-]=speed  [q]=quit" + ansi("0") + "\n";
        std::cout << std::flush;
    }

    // Accessors for external use
    ProcessManager& getProcessManager() { return process_manager; }
    MemoryManager* getMemoryManager() { return memory_manager; }
    Logger& getLogger() { return logger; }
    int getCurrentTick() const { return current_tick; }

private:
    // ----------------------------------------------------------------
    // TUI helpers
    // ----------------------------------------------------------------

    // Emit an ANSI escape code  e.g. ansi("1;32") -> "\033[1;32m"
    static std::string ansi(const std::string& code) {
        return std::string("\033[") + code + "m";
    }

    // Right-align an integer inside a field of given width
    static std::string padL(int val, int width) {
        std::string s = std::to_string(val);
        if ((int)s.size() < width) s = std::string(width - s.size(), ' ') + s;
        return s;
    }

    // Left-align a string inside a field of given width (pad right)
    static std::string padR(const std::string& s, int width) {
        if ((int)s.size() >= width) return s.substr(0, width);
        return s + std::string(width - s.size(), ' ');
    }

    static void clearScreen() {
        // ANSI: clear screen + move cursor to top-left
        std::cout << "\x1B[2J\x1B[H" << std::flush;
    }

    // Non-blocking keypress handling.
    // On Windows we use _kbhit()/_getch(); on POSIX we skip (no raw-mode setup here).
    void checkKeypress(bool& paused, bool& quit_flag, int& delay_ms) {
#ifdef _WIN32
        while (_kbhit()) {
            int ch = _getch();
            if (ch == 'q' || ch == 'Q') { quit_flag = true; return; }
            if (ch == ' ' || ch == 'p' || ch == 'P') { paused = !paused; }
            if (ch == '+' || ch == '=') { delay_ms = std::max(50,  delay_ms - 50); }
            if (ch == '-' || ch == '_') { delay_ms = std::min(2000, delay_ms + 50); }
        }
#else
        // POSIX non-blocking read would need termios; we just poll stdin with 0 timeout.
        // For simplicity leave key handling as a no-op on non-Windows.
        (void)paused; (void)quit_flag; (void)delay_ms;
#endif
    }
};
