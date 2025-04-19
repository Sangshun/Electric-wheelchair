#include "UltrasonicSensor.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

// Global flag to control the main loop
std::atomic<bool> running(true);

void signal_handler(int signum) {
    (void)signum;
    running.store(false);
}

int main() {
    // Register signal handler for Ctrl+C (SIGINT)
    std::signal(SIGINT, signal_handler);

    // Create UltrasonicSensor object with trigger pin 23 and echo pin 24
    UltrasonicSensor sensor(23, 24);

    // Set an optional callback to print the measured distance
    sensor.set_callback([](float distance) {
        std::cout << "[Callback] Measured distance: " << distance << " cm" << std::endl;
    });

    // Start the sensor measurement thread
    sensor.start();

    std::cout << "Ultrasonic sensor test program started. Press Ctrl+C to exit." << std::endl;

    // Main loop: print the distance reading every second
    while (running.load()) {
        std::string distance_str = sensor.get_distance_str();
        std::cout << "Distance: " << distance_str << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Stop the sensor thread before exiting
    sensor.stop();

    std::cout << "Test program exiting." << std::endl;
    return 0;
}
