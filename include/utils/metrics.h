#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include "../kernel/pcb.h"

// Holds computed metrics for a single simulation run
struct SchedulingMetrics {
    std::string scheduler_name;
    int total_processes;
    int total_ticks;

    double avg_waiting_time;
    double avg_turnaround_time;
    double avg_response_time;
    double cpu_utilization;        // percentage
    double throughput;             // processes per unit time
    int context_switches;

    // Per-process data
    std::vector<int> pids;
    std::vector<int> waiting_times;
    std::vector<int> turnaround_times;
    std::vector<int> response_times;
};

struct MemoryMetrics {
    std::string replacement_policy;
    int total_frames;
    int total_page_accesses;
    int total_page_faults;
    int total_page_replacements;
    double page_fault_rate;
    double frame_utilization;      // percentage of frames used on average
};

class MetricsCollector {
public:
    // Compute scheduling metrics from completed processes
    static SchedulingMetrics computeSchedulingMetrics(
        const std::string& scheduler_name,
        const std::vector<PCB>& processes,
        int total_ticks,
        int context_switches,
        int idle_ticks)
    {
        SchedulingMetrics m;
        m.scheduler_name = scheduler_name;
        m.total_processes = (int)processes.size();
        m.total_ticks = total_ticks;
        m.context_switches = context_switches;

        double sum_wait = 0, sum_turn = 0, sum_resp = 0;
        for (auto& p : processes) {
            m.pids.push_back(p.pid);
            m.waiting_times.push_back(p.waiting_time);
            m.turnaround_times.push_back(p.turnaround_time);
            m.response_times.push_back(p.response_time);

            sum_wait += p.waiting_time;
            sum_turn += p.turnaround_time;
            sum_resp += p.response_time;
        }

        int n = m.total_processes;
        m.avg_waiting_time    = n > 0 ? sum_wait / n : 0;
        m.avg_turnaround_time = n > 0 ? sum_turn / n : 0;
        m.avg_response_time   = n > 0 ? sum_resp / n : 0;
        m.cpu_utilization     = total_ticks > 0
            ? 100.0 * (total_ticks - idle_ticks) / total_ticks : 0;
        m.throughput          = total_ticks > 0
            ? (double)n / total_ticks : 0;

        return m;
    }

    // Print scheduling metrics to console
    static void printSchedulingMetrics(const SchedulingMetrics& m) {
        std::cout << "\n╔══════════════════════════════════════════════════╗\n";
        std::cout << "║       SCHEDULING METRICS: " << std::setw(20) << std::left
                  << m.scheduler_name << "  ║\n";
        std::cout << "╠══════════════════════════════════════════════════╣\n";
        std::cout << "║  Total Processes:     " << std::setw(26) << m.total_processes << "║\n";
        std::cout << "║  Total Ticks:         " << std::setw(26) << m.total_ticks << "║\n";
        std::cout << "║  Context Switches:    " << std::setw(26) << m.context_switches << "║\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "║  Avg Waiting Time:    " << std::setw(26) << m.avg_waiting_time << "║\n";
        std::cout << "║  Avg Turnaround Time: " << std::setw(26) << m.avg_turnaround_time << "║\n";
        std::cout << "║  Avg Response Time:   " << std::setw(26) << m.avg_response_time << "║\n";
        std::cout << "║  CPU Utilization:     " << std::setw(24) << m.cpu_utilization << " %║\n";
        std::cout << "║  Throughput:          " << std::setw(22) << m.throughput << " p/t║\n";
        std::cout << "╠══════════════════════════════════════════════════╣\n";
        std::cout << "║  PID  │ Wait  │ Turnaround │ Response           ║\n";
        std::cout << "║───────┼───────┼────────────┼────────────────────║\n";
        for (size_t i = 0; i < m.pids.size(); i++) {
            std::cout << "║  " << std::setw(4) << m.pids[i] << " │ "
                      << std::setw(5) << m.waiting_times[i] << " │ "
                      << std::setw(10) << m.turnaround_times[i] << " │ "
                      << std::setw(18) << m.response_times[i] << " ║\n";
        }
        std::cout << "╚══════════════════════════════════════════════════╝\n\n";
    }

    // Export metrics to JSON file
    static void exportToJSON(const SchedulingMetrics& m,
                             const std::string& filepath) {
        std::ofstream f(filepath);
        if (!f.is_open()) return;

        f << "{\n";
        f << "  \"scheduler\": \"" << m.scheduler_name << "\",\n";
        f << "  \"total_processes\": " << m.total_processes << ",\n";
        f << "  \"total_ticks\": " << m.total_ticks << ",\n";
        f << "  \"context_switches\": " << m.context_switches << ",\n";
        f << std::fixed << std::setprecision(2);
        f << "  \"avg_waiting_time\": " << m.avg_waiting_time << ",\n";
        f << "  \"avg_turnaround_time\": " << m.avg_turnaround_time << ",\n";
        f << "  \"avg_response_time\": " << m.avg_response_time << ",\n";
        f << "  \"cpu_utilization\": " << m.cpu_utilization << ",\n";
        f << "  \"throughput\": " << m.throughput << ",\n";
        f << "  \"per_process\": [\n";
        for (size_t i = 0; i < m.pids.size(); i++) {
            f << "    {\"pid\": " << m.pids[i]
              << ", \"waiting\": " << m.waiting_times[i]
              << ", \"turnaround\": " << m.turnaround_times[i]
              << ", \"response\": " << m.response_times[i] << "}";
            if (i < m.pids.size() - 1) f << ",";
            f << "\n";
        }
        f << "  ]\n}\n";
        f.close();
    }

    // Export memory metrics to JSON
    static void exportMemoryMetrics(const MemoryMetrics& m,
                                    const std::string& filepath) {
        std::ofstream f(filepath);
        if (!f.is_open()) return;

        f << "{\n";
        f << "  \"replacement_policy\": \"" << m.replacement_policy << "\",\n";
        f << "  \"total_frames\": " << m.total_frames << ",\n";
        f << "  \"total_page_accesses\": " << m.total_page_accesses << ",\n";
        f << "  \"total_page_faults\": " << m.total_page_faults << ",\n";
        f << "  \"total_page_replacements\": " << m.total_page_replacements << ",\n";
        f << std::fixed << std::setprecision(4);
        f << "  \"page_fault_rate\": " << m.page_fault_rate << ",\n";
        f << "  \"frame_utilization\": " << m.frame_utilization << "\n";
        f << "}\n";
        f.close();
    }
};
