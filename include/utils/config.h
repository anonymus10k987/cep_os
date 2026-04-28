#pragma once
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>

// Simple key-value config parser (no external YAML dependency)
// Config file format:
//   key = value
//   # comments
struct SimConfig {
    // Scheduling
    std::string scheduler = "rr";       // fcfs, rr, priority
    int time_quantum = 4;               // for Round Robin
    bool preemptive_priority = true;    // for Priority scheduler

    // Memory
    int total_physical_frames = 16;     // physical memory size in frames
    int page_size = 4;                  // KB per page
    std::string replacement_policy = "lru"; // fifo, lru
    int max_pages_per_process = 8;      // max virtual pages per process

    // Workload
    int num_processes = 5;
    int min_cpu_burst = 5;
    int max_cpu_burst = 30;
    int min_io_burst = 0;
    int max_io_burst = 10;
    int min_memory_pages = 2;
    int max_memory_pages = 8;
    int arrival_spread = 5;             // max ticks between arrivals

    // Logging
    bool console_logging = true;
    bool file_logging = true;
    std::string log_dir = "output/logs";

    // Simulation
    int random_seed = 42;
    bool interactive_mode = false;

    // Load from file
    bool loadFromFile(const std::string& filepath) {
        std::ifstream f(filepath);
        if (!f.is_open()) {
            std::cerr << "[Config] Cannot open: " << filepath << std::endl;
            return false;
        }

        std::string line;
        std::map<std::string, std::string> kv;

        while (std::getline(f, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;

            auto eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key = trim(line.substr(0, eq));
            std::string val = trim(line.substr(eq + 1));
            kv[key] = val;
        }
        f.close();

        // Map values
        if (kv.count("scheduler"))          scheduler = kv["scheduler"];
        if (kv.count("time_quantum"))       time_quantum = std::stoi(kv["time_quantum"]);
        if (kv.count("preemptive_priority")) preemptive_priority = (kv["preemptive_priority"] == "true");
        if (kv.count("total_physical_frames")) total_physical_frames = std::stoi(kv["total_physical_frames"]);
        if (kv.count("page_size"))          page_size = std::stoi(kv["page_size"]);
        if (kv.count("replacement_policy")) replacement_policy = kv["replacement_policy"];
        if (kv.count("max_pages_per_process")) max_pages_per_process = std::stoi(kv["max_pages_per_process"]);
        if (kv.count("num_processes"))      num_processes = std::stoi(kv["num_processes"]);
        if (kv.count("min_cpu_burst"))      min_cpu_burst = std::stoi(kv["min_cpu_burst"]);
        if (kv.count("max_cpu_burst"))      max_cpu_burst = std::stoi(kv["max_cpu_burst"]);
        if (kv.count("min_io_burst"))       min_io_burst = std::stoi(kv["min_io_burst"]);
        if (kv.count("max_io_burst"))       max_io_burst = std::stoi(kv["max_io_burst"]);
        if (kv.count("min_memory_pages"))   min_memory_pages = std::stoi(kv["min_memory_pages"]);
        if (kv.count("max_memory_pages"))   max_memory_pages = std::stoi(kv["max_memory_pages"]);
        if (kv.count("arrival_spread"))     arrival_spread = std::stoi(kv["arrival_spread"]);
        if (kv.count("console_logging"))    console_logging = (kv["console_logging"] == "true");
        if (kv.count("file_logging"))       file_logging = (kv["file_logging"] == "true");
        if (kv.count("log_dir"))            log_dir = kv["log_dir"];
        if (kv.count("random_seed"))        random_seed = std::stoi(kv["random_seed"]);

        return true;
    }

    void print() const {
        std::cout << "\n=== Simulation Configuration ===" << std::endl;
        std::cout << "  Scheduler:        " << scheduler << std::endl;
        std::cout << "  Time Quantum:     " << time_quantum << std::endl;
        std::cout << "  Processes:        " << num_processes << std::endl;
        std::cout << "  CPU Burst:        " << min_cpu_burst << "-" << max_cpu_burst << std::endl;
        std::cout << "  I/O Burst:        " << min_io_burst << "-" << max_io_burst << std::endl;
        std::cout << "  Memory Pages:     " << min_memory_pages << "-" << max_memory_pages << std::endl;
        std::cout << "  Physical Frames:  " << total_physical_frames << std::endl;
        std::cout << "  Replacement:      " << replacement_policy << std::endl;
        std::cout << "  Random Seed:      " << random_seed << std::endl;
        std::cout << "================================\n" << std::endl;
    }

private:
    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }
};
