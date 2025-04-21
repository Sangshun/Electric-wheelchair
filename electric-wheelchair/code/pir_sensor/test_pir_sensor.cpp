#include "pir_sensor.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

// Global flag to control the main loop.
std::atomic<bool> running(true);

void signal_handler(int signum) {
    (void)signum;
    running.store(false);
}

int main() {
    // Install signal handler for Ctrl+C.
    std::signal(SIGINT, signal_handler);

    // Create a PIRController instance on chip "gpiochip0" and GPIO pin 25.
    // The callback prints "Motion detected" on a rising edge and "No motion" on a falling edge.
    PIRController pirSensor("gpiochip0", 25, [](bool motion) {
        if (motion) {
            std::cout << "Motion detected" << std::endl;
        } else {
            std::cout << "No motion" << std::endl;
        }
    });

    // Start the PIR sensor monitoring with a debounce time (e.g., 200 ms).
    pirSensor.start(200);

    std::cout << "PIR sensor test running. Press Ctrl+C to exit." << std::endl;

    // Main loop: wait until Ctrl+C is pressed.
    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    pirSensor.stop();
    std::cout << "Exiting PIR sensor test program." << std::endl;
    return 0;
}
