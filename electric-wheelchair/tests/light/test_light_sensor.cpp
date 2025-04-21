#include "LightSensor.hpp"
#include <iostream>
#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>

// Global flag to control the main loop
std::atomic<bool> running(true);

// Signal handler to gracefully stop the test on Ctrl+C
void signal_handler(int signal) {
    if (signal == SIGINT) {
        running.store(false);
    }
}

int main() {
    // Register signal handler for SIGINT
    std::signal(SIGINT, signal_handler);

    std::cout << "Light Sensor test program started. Press Ctrl+C to exit." << std::endl;

    // Create LightSensor object on GPIO pin 16 with a callback that prints the sensor state
    LightSensor sensor(16, [](bool state) {
        std::cout << "Light Sensor State: " << (state ? "Dark" : "Light") << std::endl;
    });

    // Start the light sensor event monitoring thread
    sensor.start();

    // Main loop: simply wait until Ctrl+C is pressed
    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Stop the sensor thread before exiting
    sensor.stop();

    std::cout << "Light Sensor test program exiting." << std::endl;
    return 0;
}
