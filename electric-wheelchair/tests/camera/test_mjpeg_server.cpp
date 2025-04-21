#include "mjpeg_server.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

std::atomic<bool> running(true);

void signal_handler(int signum) {
    (void)signum;
    running.store(false);
}

int main() {
    std::signal(SIGINT, signal_handler);

    MJPEGServer server(8080);
    if (!server.start()) {
        std::cerr << "Failed to start MJPEGServer" << std::endl;
        return 1;
    }
    std::cout << "MJPEGServer running on port 8080. Press Ctrl+C to stop." << std::endl;

    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Stopping MJPEGServer..." << std::endl;
    server.stop();
    std::cout << "MJPEGServer stopped." << std::endl;
    return 0;
}
