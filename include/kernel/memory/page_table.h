#pragma once
#include <vector>
#include <iostream>
#include <iomanip>

// Page Table Entry — represents one virtual-to-physical page mapping
struct PageTableEntry {
    int frame_number;       // Physical frame (-1 if not in memory)
    bool valid;             // Is this page currently in physical memory?
    bool dirty;             // Has this page been modified?
    bool referenced;        // Has this page been accessed recently? (for LRU)
    int last_access_time;   // Tick of last access (for LRU)
    int load_time;          // Tick when loaded into memory (for FIFO)

    PageTableEntry()
        : frame_number(-1), valid(false), dirty(false),
          referenced(false), last_access_time(0), load_time(0) {}
};

// Page Table — per-process mapping of virtual pages to physical frames
class PageTable {
private:
    int owner_pid;
    int num_pages;                          // virtual address space size in pages
    std::vector<PageTableEntry> entries;

public:
    PageTable() : owner_pid(-1), num_pages(0) {}

    explicit PageTable(int pid, int pages)
        : owner_pid(pid), num_pages(pages), entries(pages) {}

    // Access a page — returns frame number or -1 (page fault)
    int accessPage(int page_number, int current_tick, bool is_write = false) {
        if (page_number < 0 || page_number >= num_pages) return -1;

        PageTableEntry& entry = entries[page_number];
        if (!entry.valid) {
            return -1; // PAGE FAULT
        }

        // Update access metadata
        entry.referenced = true;
        entry.last_access_time = current_tick;
        if (is_write) entry.dirty = true;

        return entry.frame_number;
    }

    // Map a virtual page to a physical frame
    void mapPage(int page_number, int frame_number, int current_tick) {
        if (page_number < 0 || page_number >= num_pages) return;

        entries[page_number].frame_number = frame_number;
        entries[page_number].valid = true;
        entries[page_number].dirty = false;
        entries[page_number].referenced = true;
        entries[page_number].last_access_time = current_tick;
        entries[page_number].load_time = current_tick;
    }

    // Unmap a page (used during page replacement)
    void unmapPage(int page_number) {
        if (page_number < 0 || page_number >= num_pages) return;

        entries[page_number].frame_number = -1;
        entries[page_number].valid = false;
        entries[page_number].dirty = false;
        entries[page_number].referenced = false;
    }

    // Get entry for inspection
    const PageTableEntry& getEntry(int page_number) const {
        return entries[page_number];
    }

    PageTableEntry& getEntryMut(int page_number) {
        return entries[page_number];
    }

    // Get all valid (resident) pages
    std::vector<int> getResidentPages() const {
        std::vector<int> result;
        for (int i = 0; i < num_pages; i++) {
            if (entries[i].valid) result.push_back(i);
        }
        return result;
    }

    int getOwnerPid() const { return owner_pid; }
    int getNumPages() const { return num_pages; }
    int getResidentCount() const {
        int c = 0;
        for (auto& e : entries) if (e.valid) c++;
        return c;
    }

    // Print page table
    void print() const {
        std::cout << "Page Table (PID=" << owner_pid << ", pages=" << num_pages << "):\n";
        std::cout << "  Page │ Frame │ Valid │ Dirty │ Ref │ LastAccess │ LoadTime\n";
        std::cout << "  ─────┼───────┼───────┼───────┼─────┼────────────┼─────────\n";
        for (int i = 0; i < num_pages; i++) {
            auto& e = entries[i];
            std::cout << "  " << std::setw(4) << i << " │ "
                      << std::setw(5) << (e.valid ? std::to_string(e.frame_number) : "  -") << " │ "
                      << std::setw(5) << (e.valid ? "  Y" : "  N") << " │ "
                      << std::setw(5) << (e.dirty ? "  Y" : "  N") << " │ "
                      << std::setw(3) << (e.referenced ? " Y" : " N") << " │ "
                      << std::setw(10) << e.last_access_time << " │ "
                      << std::setw(8) << e.load_time << "\n";
        }
    }
};
