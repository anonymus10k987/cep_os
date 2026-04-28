#pragma once
#include <vector>
#include <iostream>

// Physical memory frame allocator — manages a bitmap of free/allocated frames
class FrameAllocator {
private:
    int total_frames;
    std::vector<bool> frame_map;     // true = allocated, false = free
    int allocated_count;

public:
    explicit FrameAllocator(int frames = 16)
        : total_frames(frames), frame_map(frames, false), allocated_count(0) {}

    // Allocate a free frame, returns frame number or -1 if full
    int allocate() {
        for (int i = 0; i < total_frames; i++) {
            if (!frame_map[i]) {
                frame_map[i] = true;
                allocated_count++;
                return i;
            }
        }
        return -1; // no free frames
    }

    // Deallocate a frame
    void deallocate(int frame) {
        if (frame >= 0 && frame < total_frames && frame_map[frame]) {
            frame_map[frame] = false;
            allocated_count--;
        }
    }

    // Check if a frame is allocated
    bool isAllocated(int frame) const {
        if (frame < 0 || frame >= total_frames) return false;
        return frame_map[frame];
    }

    int getTotalFrames() const { return total_frames; }
    int getAllocatedCount() const { return allocated_count; }
    int getFreeCount() const { return total_frames - allocated_count; }
    bool isFull() const { return allocated_count >= total_frames; }

    // Get utilization as percentage
    double getUtilization() const {
        return total_frames > 0 ? 100.0 * allocated_count / total_frames : 0.0;
    }

    void print() const {
        std::cout << "Frames [" << allocated_count << "/" << total_frames << "]: ";
        for (int i = 0; i < total_frames; i++) {
            std::cout << (frame_map[i] ? "█" : "░");
        }
        std::cout << std::endl;
    }
};
