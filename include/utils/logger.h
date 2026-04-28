#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <mutex>
#include <iomanip>

// Log categories for structured output
enum class LogCategory {
    KERNEL,
    SCHED,
    SYNC,
    MEM,
    PROC,
    IO,
    METRIC
};

inline std::string categoryToString(LogCategory c) {
    switch (c) {
        case LogCategory::KERNEL: return "KERNEL";
        case LogCategory::SCHED:  return "SCHED ";
        case LogCategory::SYNC:   return "SYNC  ";
        case LogCategory::MEM:    return "MEM   ";
        case LogCategory::PROC:   return "PROC  ";
        case LogCategory::IO:     return "IO    ";
        case LogCategory::METRIC: return "METRIC";
        default:                  return "???   ";
    }
}

// Structured event for CSV export
struct LogEvent {
    int tick;
    LogCategory category;
    std::string message;
    std::string details; // extra CSV-friendly data
};

class Logger {
private:
    std::vector<LogEvent> events;
    bool console_output;
    bool file_output;
    std::string log_dir;
    std::mutex log_mutex;

    // CSV files for different subsystems
    std::ofstream scheduling_csv;
    std::ofstream memory_csv;
    std::ofstream sync_csv;

public:
    Logger(bool console = true, bool file = true,
           const std::string& dir = "output/logs")
        : console_output(console), file_output(file), log_dir(dir) {}

    // Initialize log files
    bool init() {
        if (!file_output) return true;

        scheduling_csv.open(log_dir + "/scheduling_log.csv");
        memory_csv.open(log_dir + "/memory_log.csv");
        sync_csv.open(log_dir + "/sync_log.csv");

        if (!scheduling_csv.is_open() || !memory_csv.is_open() || !sync_csv.is_open()) {
            std::cerr << "[Logger] Failed to open log files in " << log_dir << std::endl;
            return false;
        }

        // Write CSV headers
        scheduling_csv << "tick,event,pid,process_name,scheduler,detail\n";
        memory_csv << "tick,event,pid,page,frame,policy,detail\n";
        sync_csv << "tick,event,tid,primitive,id,detail\n";

        return true;
    }

    // Main log function
    void log(int tick, LogCategory category, const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);

        LogEvent event{tick, category, message, ""};
        events.push_back(event);

        if (console_output) {
            std::cout << "[TICK " << std::setw(4) << std::setfill('0') << tick
                      << "] [" << categoryToString(category) << "] "
                      << message << std::endl;
        }
    }

    // Scheduling-specific CSV log
    void logScheduling(int tick, const std::string& event,
                       int pid, const std::string& process_name,
                       const std::string& scheduler,
                       const std::string& detail = "") {
        std::lock_guard<std::mutex> lock(log_mutex);

        if (file_output && scheduling_csv.is_open()) {
            scheduling_csv << tick << "," << event << "," << pid << ","
                          << process_name << "," << scheduler << ","
                          << detail << "\n";
        }

        if (console_output) {
            std::cout << "[TICK " << std::setw(4) << std::setfill('0') << tick
                      << "] [SCHED ] " << event << ": " << process_name
                      << " (PID=" << pid << ") " << detail << std::endl;
        }
    }

    // Memory-specific CSV log
    void logMemory(int tick, const std::string& event,
                   int pid, int page, int frame,
                   const std::string& policy,
                   const std::string& detail = "") {
        std::lock_guard<std::mutex> lock(log_mutex);

        if (file_output && memory_csv.is_open()) {
            memory_csv << tick << "," << event << "," << pid << ","
                      << page << "," << frame << "," << policy << ","
                      << detail << "\n";
        }

        if (console_output) {
            std::cout << "[TICK " << std::setw(4) << std::setfill('0') << tick
                      << "] [MEM   ] " << event << ": PID=" << pid
                      << " page=" << page << " frame=" << frame
                      << " (" << policy << ") " << detail << std::endl;
        }
    }

    // Sync-specific CSV log
    void logSync(int tick, const std::string& event,
                 int tid, const std::string& primitive,
                 int prim_id, const std::string& detail = "") {
        std::lock_guard<std::mutex> lock(log_mutex);

        if (file_output && sync_csv.is_open()) {
            sync_csv << tick << "," << event << "," << tid << ","
                    << primitive << "," << prim_id << ","
                    << detail << "\n";
        }

        if (console_output) {
            std::cout << "[TICK " << std::setw(4) << std::setfill('0') << tick
                      << "] [SYNC  ] " << event << ": TID=" << tid
                      << " " << primitive << "#" << prim_id
                      << " " << detail << std::endl;
        }
    }

    // Flush and close all files
    void close() {
        if (scheduling_csv.is_open()) { scheduling_csv.flush(); scheduling_csv.close(); }
        if (memory_csv.is_open())     { memory_csv.flush(); memory_csv.close(); }
        if (sync_csv.is_open())       { sync_csv.flush(); sync_csv.close(); }
    }

    // Get all events (for metrics processing)
    const std::vector<LogEvent>& getEvents() const { return events; }

    ~Logger() { close(); }
};
