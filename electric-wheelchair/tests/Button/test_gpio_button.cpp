#include "GPIOButton.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

std::atomic<bool> running(true);

void signal_handler(int signum) {
    (void) signum;
    running.store(false);
}

int main() {
    // Install signal handler for Ctrl+C
    std::signal(SIGINT, signal_handler);

    // Create a GPIOButton instance on pin 5.
    // Short press callback prints "Short press detected."
    // Long press callback prints "Long press detected."
    GPIOButton button(5,
        []() { std::cout << "Short press detected." << std::endl; },
        []() { std::cout << "Long press detected." << std::endl; }
    );

    if (!button.start()) {
        std::cerr << "Failed to start GPIOButton." << std::endl;
        return 1;
    }

    std::cout << "GPIOButton test running. Press Ctrl+C to exit." << std::endl;
    
    // Main loop runs until Ctrl+C is pressed
    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    button.stop();
    std::cout << "Exiting GPIOButton test program." << std::endl;
    return 0;
}
