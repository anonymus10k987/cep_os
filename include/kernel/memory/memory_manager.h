#pragma once
#include "page_table.h"
#include "frame_allocator.h"
#include "../../utils/logger.h"
#include <map>
#include <queue>
#include <vector>
#include <algorithm>
#include <string>

// Page replacement policy interface
class ReplacementPolicy {
public:
    virtual ~ReplacementPolicy() = default;
    // Select a page to evict. Returns {pid, page_number}
    virtual std::pair<int, int> selectVictim(
        const std::map<int, PageTable>& page_tables) = 0;
    // Notify policy that a page was loaded
    virtual void pageLoaded(int pid, int page, int tick) = 0;
    // Notify policy that a page was accessed
    virtual void pageAccessed(int pid, int page, int tick) = 0;
    virtual std::string name() const = 0;
};

// FIFO Page Replacement — evicts the page that was loaded earliest
class FIFOReplacementPolicy : public ReplacementPolicy {
private:
    // Queue of (pid, page) in load order
    std::queue<std::pair<int, int>> load_order;

public:
    std::pair<int, int> selectVictim(
        const std::map<int, PageTable>& page_tables) override
    {
        while (!load_order.empty()) {
            auto [pid, page] = load_order.front();
            load_order.pop();
            // Verify it's still valid (wasn't already evicted)
            auto it = page_tables.find(pid);
            if (it != page_tables.end()) {
                const auto& entry = it->second.getEntry(page);
                if (entry.valid) {
                    return {pid, page};
                }
            }
        }
        return {-1, -1}; // shouldn't happen if memory is full
    }

    void pageLoaded(int pid, int page, int tick) override {
        load_order.push({pid, page});
    }

    void pageAccessed(int pid, int page, int tick) override {
        // FIFO doesn't care about accesses
    }

    std::string name() const override { return "FIFO"; }
};

// LRU Page Replacement — evicts the Least Recently Used page
class LRUReplacementPolicy : public ReplacementPolicy {
public:
    std::pair<int, int> selectVictim(
        const std::map<int, PageTable>& page_tables) override
    {
        int oldest_tick = INT_MAX;
        int victim_pid = -1, victim_page = -1;

        for (auto& [pid, pt] : page_tables) {
            for (int p = 0; p < pt.getNumPages(); p++) {
                const auto& entry = pt.getEntry(p);
                if (entry.valid && entry.last_access_time < oldest_tick) {
                    oldest_tick = entry.last_access_time;
                    victim_pid = pid;
                    victim_page = p;
                }
            }
        }
        return {victim_pid, victim_page};
    }

    void pageLoaded(int pid, int page, int tick) override {
        // LRU tracks via last_access_time in PageTableEntry
    }

    void pageAccessed(int pid, int page, int tick) override {
        // Tracked via PageTableEntry.last_access_time
    }

    std::string name() const override { return "LRU"; }
};

// Memory Manager — coordinates page tables, frame allocation, and replacement
class MemoryManager {
private:
    FrameAllocator frame_allocator;
    std::map<int, PageTable> page_tables;     // pid -> page table
    ReplacementPolicy* replacement_policy;
    Logger* logger;

    // Metrics
    int total_page_accesses = 0;
    int total_page_faults = 0;
    int total_replacements = 0;

public:
    MemoryManager(int total_frames, ReplacementPolicy* policy, Logger* log = nullptr)
        : frame_allocator(total_frames), replacement_policy(policy), logger(log) {}

    // Initialize a page table for a new process
    void initProcess(int pid, int num_pages) {
        page_tables[pid] = PageTable(pid, num_pages);
        if (logger) {
            logger->log(0, LogCategory::MEM,
                "Initialized page table for PID=" + std::to_string(pid) +
                " (" + std::to_string(num_pages) + " virtual pages)");
        }
    }

    // Access a virtual page — handles page faults and replacement
    // Returns the physical frame number
    int accessPage(int pid, int page_number, int current_tick, bool is_write = false) {
        total_page_accesses++;

        auto it = page_tables.find(pid);
        if (it == page_tables.end()) return -1; // no page table for this process

        PageTable& pt = it->second;
        int frame = pt.accessPage(page_number, current_tick, is_write);

        if (frame >= 0) {
            // Page hit
            replacement_policy->pageAccessed(pid, page_number, current_tick);
            return frame;
        }

        // PAGE FAULT
        total_page_faults++;
        return handlePageFault(pid, page_number, current_tick);
    }

    // Handle a page fault — load page into memory, possibly evicting another
    int handlePageFault(int pid, int page_number, int current_tick) {
        int frame = frame_allocator.allocate();

        if (frame < 0) {
            // No free frames — need to replace
            total_replacements++;
            auto [victim_pid, victim_page] = replacement_policy->selectVictim(page_tables);

            if (victim_pid < 0) {
                if (logger) logger->log(current_tick, LogCategory::MEM,
                    "CRITICAL: No victim found for replacement!");
                return -1;
            }

            // Get the frame from victim
            frame = page_tables[victim_pid].getEntry(victim_page).frame_number;

            if (logger) {
                logger->logMemory(current_tick, "PAGE_REPLACE", pid, page_number, frame,
                    replacement_policy->name(),
                    "evicted PID=" + std::to_string(victim_pid) +
                    ":page" + std::to_string(victim_page));
            }

            // Unmap victim page
            page_tables[victim_pid].unmapPage(victim_page);
        } else {
            if (logger) {
                logger->logMemory(current_tick, "PAGE_FAULT", pid, page_number, frame,
                    replacement_policy->name(), "loaded into free frame");
            }
        }

        // Map the new page
        page_tables[pid].mapPage(page_number, frame, current_tick);
        replacement_policy->pageLoaded(pid, page_number, current_tick);

        return frame;
    }

    // Deallocate all pages for a terminated process
    void deallocateProcess(int pid) {
        auto it = page_tables.find(pid);
        if (it == page_tables.end()) return;

        PageTable& pt = it->second;
        for (int p = 0; p < pt.getNumPages(); p++) {
            if (pt.getEntry(p).valid) {
                frame_allocator.deallocate(pt.getEntry(p).frame_number);
                pt.unmapPage(p);
            }
        }
    }

    // Get page table for a process
    PageTable* getPageTable(int pid) {
        auto it = page_tables.find(pid);
        return it != page_tables.end() ? &it->second : nullptr;
    }

    const std::map<int, PageTable>& getAllPageTables() const { return page_tables; }

    // Metrics getters
    int getTotalPageAccesses() const { return total_page_accesses; }
    int getTotalPageFaults() const { return total_page_faults; }
    int getTotalReplacements() const { return total_replacements; }
    double getPageFaultRate() const {
        return total_page_accesses > 0
            ? (double)total_page_faults / total_page_accesses : 0.0;
    }
    double getFrameUtilization() const { return frame_allocator.getUtilization(); }

    const FrameAllocator& getFrameAllocator() const { return frame_allocator; }

    void printStatus() const {
        std::cout << "\n=== Memory Status ===" << std::endl;
        frame_allocator.print();
        std::cout << "Page Accesses: " << total_page_accesses
                  << "  Faults: " << total_page_faults
                  << "  Replacements: " << total_replacements
                  << "  Fault Rate: " << std::fixed << std::setprecision(2)
                  << (getPageFaultRate() * 100) << "%" << std::endl;
    }
};
