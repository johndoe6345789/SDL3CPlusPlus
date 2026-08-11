/**
 * Spy Thread Debugger - Demo Program
 *
 * This demonstrates a main program that updates atomic variables
 * while a spy thread listens on a socket for queries.
 *
 * Usage:
 *   1. Compile and run:
 *      clang++ -std=c++17 -pthread spy_thread_debugger.cpp spy_thread_demo.cpp -o spy_demo
 *      ./spy_demo
 *
 *   2. In another terminal, connect and query:
 *      nc localhost 9999
 *      > get fps
 *      < fps=60.1
 *      > status
 *      < frame_count=600
 *      < elapsed_time=10.0
 *      < ...
 *
 * This is a lightweight way to inspect game state without blocking the main thread!
 */

#include "spy_thread_debugger.cpp"
#include <iostream>
#include <chrono>
#include <cmath>

int main() {
    std::cout << "=== Spy Thread Debugger Demo ===\n";
    std::cout << "Starting spy thread on localhost:9999...\n";

    // Create and start spy thread
    SpyThreadDebugger spy("127.0.0.1", 9999);
    if (!spy.start()) {
        std::cerr << "Failed to start spy thread\n";
        return 1;
    }

    std::cout << "\n✓ Spy thread is listening!\n";
    std::cout << "Connect with: nc localhost 9999\n";
    std::cout << "Or: telnet localhost 9999\n\n";

    // Simulate main game loop
    uint64_t frame_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    auto last_fps_time = start_time;
    uint64_t frames_this_second = 0;

    std::cout << "Running simulation for 30 seconds...\n";
    std::cout << "Frame | Elapsed | FPS   | Memory | Draw Calls | Triangles\n";
    std::cout << "------|---------|-------|--------|------------|----------\n";

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time).count();

        // Stop after 30 seconds
        if (elapsed > 30.0) {
            break;
        }

        frame_count++;
        frames_this_second++;

        // Calculate FPS
        double time_since_fps = std::chrono::duration<double>(now - last_fps_time).count();
        float fps = 0;
        if (time_since_fps >= 1.0) {
            fps = frames_this_second / time_since_fps;
            frames_this_second = 0;
            last_fps_time = now;
        } else {
            fps = frame_count / elapsed;
        }

        // Simulate game stats that vary over time
        uint32_t draw_calls = 121;  // 11x11 cube grid
        uint32_t triangles = 121 * 6 * 2;  // 6 faces, 2 triangles per face

        // Simulate varying memory usage (sine wave between 400-600 MB)
        size_t memory_base = 500 * 1024 * 1024;
        size_t memory_variance = 50 * 1024 * 1024;
        size_t memory_used = memory_base + memory_variance * std::sin(elapsed);

        // Simulate GPU time (12-18ms, averaging 15ms)
        double gpu_time = 15.0 + 3.0 * std::sin(elapsed * 2);
        double cpu_time = 12.0 + 2.0 * std::sin(elapsed * 3);

        // Update spy thread atomics (lock-free)
        spy.update_frame_count(frame_count);
        spy.update_elapsed_time(elapsed);
        spy.update_fps(fps);
        spy.update_memory(memory_used);
        spy.update_draw_calls(draw_calls);
        spy.update_triangles(triangles);
        spy.update_gpu_time(gpu_time);
        spy.update_cpu_time(cpu_time);

        // Print current stats to console
        if (frame_count % 60 == 0) {
            printf("%5llu | %7.1f | %5.1f | %5.1fMB | %10u | %9u\n",
                   frame_count, elapsed, fps,
                   memory_used / (1024.0 * 1024.0),
                   draw_calls, triangles);
        }

        // Check if external spy thread requested pause
        if (spy.paused.load()) {
            std::cout << "\n[MAIN] Paused by external command\n";
            while (spy.paused.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            std::cout << "[MAIN] Resumed by external command\n\n";
        }

        // Simulate frame time (~60 FPS = 16.67ms per frame)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::cout << "\n=== Simulation Complete ===\n";
    std::cout << "Total frames: " << frame_count << "\n";
    std::cout << "Stopping spy thread...\n";

    spy.stop();

    std::cout << "✓ Demo complete\n";
    return 0;
}
