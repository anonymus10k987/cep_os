#pragma once
#include "../kernel/pcb.h"
#include "../kernel/process_manager.h"
#include "../utils/config.h"
#include <vector>
#include <random>
#include <string>

// Generates realistic workloads based on configuration
class WorkloadGenerator {
private:
    std::mt19937 rng;

    int randRange(int min_val, int max_val) {
        if (min_val >= max_val) return min_val;
        std::uniform_int_distribution<int> dist(min_val, max_val);
        return dist(rng);
    }

public:
    explicit WorkloadGenerator(int seed = 42) : rng(seed) {}

    // Generate processes and add them to the process manager
    void generate(ProcessManager& pm, const SimConfig& config) {
        int time = 0;
        for (int i = 0; i < config.num_processes; i++) {
            std::string name = "P" + std::to_string(i + 1);
            int priority = randRange(1, 10);
            int cpu_burst = randRange(config.min_cpu_burst, config.max_cpu_burst);
            int io_burst = randRange(config.min_io_burst, config.max_io_burst);
            int mem_pages = randRange(config.min_memory_pages, config.max_memory_pages);

            pm.createProcess(name, priority, time, cpu_burst, io_burst, mem_pages);

            // Stagger arrivals
            time += randRange(0, config.arrival_spread);
        }
    }

    // Generate a CPU-bound workload (high CPU, low I/O)
    void generateCPUBound(ProcessManager& pm, int num_processes) {
        SimConfig cfg;
        cfg.num_processes = num_processes;
        cfg.min_cpu_burst = 50;
        cfg.max_cpu_burst = 200;
        cfg.min_io_burst = 0;
        cfg.max_io_burst = 5;
        cfg.min_memory_pages = 4;
        cfg.max_memory_pages = 16;
        cfg.arrival_spread = 3;
        generate(pm, cfg);
    }

    // Generate an I/O-bound workload (low CPU, high I/O)
    void generateIOBound(ProcessManager& pm, int num_processes) {
        SimConfig cfg;
        cfg.num_processes = num_processes;
        cfg.min_cpu_burst = 5;
        cfg.max_cpu_burst = 20;
        cfg.min_io_burst = 20;
        cfg.max_io_burst = 100;
        cfg.min_memory_pages = 2;
        cfg.max_memory_pages = 8;
        cfg.arrival_spread = 2;
        generate(pm, cfg);
    }

    // Generate a mixed workload
    void generateMixed(ProcessManager& pm, int num_processes) {
        SimConfig cfg;
        cfg.num_processes = num_processes;
        cfg.min_cpu_burst = 10;
        cfg.max_cpu_burst = 100;
        cfg.min_io_burst = 10;
        cfg.max_io_burst = 50;
        cfg.min_memory_pages = 4;
        cfg.max_memory_pages = 32;
        cfg.arrival_spread = 5;
        generate(pm, cfg);
    }

    // Generate a specific reference string for page replacement testing
    std::vector<std::pair<int, int>> generatePageReferenceString(
        int pid, int num_pages, int length)
    {
        std::vector<std::pair<int, int>> refs;
        for (int i = 0; i < length; i++) {
            int page = randRange(0, num_pages - 1);
            refs.push_back({pid, page});
        }
        return refs;
    }
};
