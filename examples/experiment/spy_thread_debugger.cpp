/**
 * Spy Thread Debugger
 *
 * Architecture:
 * - Main thread: Renders, updates game state, writes to atomic variables
 * - Spy thread: Listens on socket, reads atomic variables, responds to commands
 * - Communication: Lock-free (atomic<>) for state sharing, sockets for commands
 *
 * Usage:
 *   SpyThread spy("localhost", 9999);
 *   spy.start();
 *
 *   // Main program runs normally
 *   while (running) {
 *       spy.update_frame_count(frame_num);
 *       spy.update_elapsed_time(elapsed);
 *       // ... render ...
 *   }
 *
 *   spy.stop();
 *
 * Client connection:
 *   nc localhost 9999
 *   > get frame_count
 *   < frame_count=120
 *   > get memory_usage
 *   < memory_usage=45.2
 *   > pause
 *   > resume
 *   > list_commands
 */

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

class SpyThreadDebugger {
public:
    // Game state captured from main thread (lock-free)
    std::atomic<uint64_t> frame_count{0};
    std::atomic<double> elapsed_time{0.0};
    std::atomic<double> gpu_time{0.0};
    std::atomic<double> cpu_time{0.0};
    std::atomic<float> fps{0.0f};
    std::atomic<bool> paused{false};
    std::atomic<size_t> memory_used{0};
    std::atomic<uint32_t> draw_calls{0};
    std::atomic<uint32_t> triangles_rendered{0};

private:
    std::thread spy_thread_;
    std::atomic<bool> running_{false};
    int server_socket_ = -1;
    int client_socket_ = -1;
    std::string host_;
    uint16_t port_;

public:
    SpyThreadDebugger(const std::string& host = "localhost", uint16_t port = 9999)
        : host_(host), port_(port) {}

    ~SpyThreadDebugger() {
        stop();
    }

    bool start() {
        if (running_.exchange(true)) {
            return false;  // Already running
        }

        // Create server socket
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0) {
            std::cerr << "[SPY] Failed to create socket\n";
            running_ = false;
            return false;
        }

        // Set socket to reuse address
        int opt = 1;
        if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "[SPY] setsockopt failed\n";
            close(server_socket_);
            running_ = false;
            return false;
        }

        // Bind socket
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        inet_pton(AF_INET, host_.c_str(), &addr.sin_addr);

        if (bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "[SPY] Bind failed on " << host_ << ":" << port_ << "\n";
            close(server_socket_);
            running_ = false;
            return false;
        }

        // Listen for connections
        listen(server_socket_, 1);
        std::cout << "[SPY] Server listening on " << host_ << ":" << port_ << "\n";

        // Start spy thread
        spy_thread_ = std::thread([this]() { spy_loop(); });

        return true;
    }

    void stop() {
        running_ = false;

        if (client_socket_ >= 0) {
            close(client_socket_);
            client_socket_ = -1;
        }

        if (server_socket_ >= 0) {
            close(server_socket_);
            server_socket_ = -1;
        }

        if (spy_thread_.joinable()) {
            spy_thread_.join();
        }

        std::cout << "[SPY] Debugger stopped\n";
    }

    // Update methods called from main thread
    void update_frame_count(uint64_t count) {
        frame_count.store(count, std::memory_order_relaxed);
    }

    void update_elapsed_time(double seconds) {
        elapsed_time.store(seconds, std::memory_order_relaxed);
    }

    void update_gpu_time(double ms) {
        gpu_time.store(ms, std::memory_order_relaxed);
    }

    void update_cpu_time(double ms) {
        cpu_time.store(ms, std::memory_order_relaxed);
    }

    void update_fps(float fps_val) {
        fps.store(fps_val, std::memory_order_relaxed);
    }

    void update_memory(size_t bytes) {
        memory_used.store(bytes, std::memory_order_relaxed);
    }

    void update_draw_calls(uint32_t count) {
        draw_calls.store(count, std::memory_order_relaxed);
    }

    void update_triangles(uint32_t count) {
        triangles_rendered.store(count, std::memory_order_relaxed);
    }

private:
    void spy_loop() {
        std::cout << "[SPY] Thread started, waiting for connections...\n";

        while (running_) {
            // Accept client connection (non-blocking via timeout)
            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            setsockopt(server_socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            client_socket_ = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);

            if (client_socket_ < 0) {
                // No connection, continue
                continue;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            std::cout << "[SPY] Client connected from " << client_ip << "\n";

            // Handle client commands
            handle_client();

            close(client_socket_);
            client_socket_ = -1;
            std::cout << "[SPY] Client disconnected\n";
        }
    }

    void handle_client() {
        char buffer[1024];

        while (running_) {
            memset(buffer, 0, sizeof(buffer));

            // Read command from client
            ssize_t bytes = recv(client_socket_, buffer, sizeof(buffer) - 1, 0);

            if (bytes <= 0) {
                break;  // Client disconnected
            }

            std::string command(buffer);
            // Remove trailing newline
            if (!command.empty() && command.back() == '\n') {
                command.pop_back();
            }

            std::string response = process_command(command);
            send(client_socket_, response.c_str(), response.length(), 0);
        }
    }

    std::string process_command(const std::string& cmd) {
        std::istringstream iss(cmd);
        std::string action, param;
        iss >> action >> param;

        if (action == "get") {
            return get_stat(param);
        } else if (action == "status") {
            return get_status();
        } else if (action == "pause") {
            paused.store(true);
            return "paused=true\n";
        } else if (action == "resume") {
            paused.store(false);
            return "paused=false\n";
        } else if (action == "list_commands") {
            return get_commands_list();
        } else if (action == "help") {
            return get_help();
        } else {
            return "error=unknown_command\n";
        }
    }

    std::string get_stat(const std::string& stat_name) {
        std::ostringstream oss;

        if (stat_name == "frame_count") {
            oss << "frame_count=" << frame_count.load(std::memory_order_relaxed) << "\n";
        } else if (stat_name == "elapsed_time") {
            oss << "elapsed_time=" << elapsed_time.load(std::memory_order_relaxed) << "\n";
        } else if (stat_name == "gpu_time") {
            oss << "gpu_time=" << gpu_time.load(std::memory_order_relaxed) << "\n";
        } else if (stat_name == "cpu_time") {
            oss << "cpu_time=" << cpu_time.load(std::memory_order_relaxed) << "\n";
        } else if (stat_name == "fps") {
            oss << "fps=" << fps.load(std::memory_order_relaxed) << "\n";
        } else if (stat_name == "memory") {
            oss << "memory_used=" << memory_used.load(std::memory_order_relaxed) << "\n";
        } else if (stat_name == "draw_calls") {
            oss << "draw_calls=" << draw_calls.load(std::memory_order_relaxed) << "\n";
        } else if (stat_name == "triangles") {
            oss << "triangles_rendered=" << triangles_rendered.load(std::memory_order_relaxed) << "\n";
        } else if (stat_name == "all") {
            return get_status();
        } else {
            oss << "error=unknown_stat\n";
        }

        return oss.str();
    }

    std::string get_status() {
        std::ostringstream oss;
        oss << "frame_count=" << frame_count.load(std::memory_order_relaxed) << "\n";
        oss << "elapsed_time=" << elapsed_time.load(std::memory_order_relaxed) << "\n";
        oss << "fps=" << fps.load(std::memory_order_relaxed) << "\n";
        oss << "gpu_time=" << gpu_time.load(std::memory_order_relaxed) << "\n";
        oss << "cpu_time=" << cpu_time.load(std::memory_order_relaxed) << "\n";
        oss << "memory_used=" << memory_used.load(std::memory_order_relaxed) << "\n";
        oss << "draw_calls=" << draw_calls.load(std::memory_order_relaxed) << "\n";
        oss << "triangles_rendered=" << triangles_rendered.load(std::memory_order_relaxed) << "\n";
        oss << "paused=" << (paused.load() ? "true" : "false") << "\n";
        return oss.str();
    }

    std::string get_commands_list() {
        return "get <stat>       - Get single stat (frame_count, fps, memory, etc)\n"
               "status           - Get all stats\n"
               "pause            - Pause main thread (set paused flag)\n"
               "resume           - Resume main thread\n"
               "list_commands    - Show this list\n"
               "help             - Show full help\n";
    }

    std::string get_help() {
        std::ostringstream oss;
        oss << "=== Spy Thread Debugger ===\n"
            << "Connect: nc localhost 9999\n"
            << "\n"
            << "Available stats:\n"
            << "  frame_count      - Current frame number\n"
            << "  elapsed_time     - Seconds since start\n"
            << "  fps              - Frames per second\n"
            << "  gpu_time         - GPU frame time in ms\n"
            << "  cpu_time         - CPU frame time in ms\n"
            << "  memory           - Memory used in bytes\n"
            << "  draw_calls       - Number of draw calls\n"
            << "  triangles        - Triangles rendered\n"
            << "\n"
            << "Commands:\n"
            << "  get <stat>       - Query specific stat\n"
            << "  get all          - Get all stats\n"
            << "  status           - Alias for 'get all'\n"
            << "  pause            - Set paused flag (main thread reads it)\n"
            << "  resume           - Clear paused flag\n"
            << "  list_commands    - List available commands\n"
            << "  help             - Show this help\n"
            << "\n"
            << "Example:\n"
            << "  nc localhost 9999\n"
            << "  > get fps\n"
            << "  < fps=60.5\n"
            << "  > status\n"
            << "  < frame_count=1200\n"
            << "  < elapsed_time=20.0\n"
            << "  < ...\n";
        return oss.str();
    }
};
