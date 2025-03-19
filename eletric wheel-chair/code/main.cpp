#include "GPIOButton.hpp"
#include "MotorController.hpp"
#include "LightSensor.hpp"
#include <iostream>
#include <csignal>
#include <chrono>
#include <thread>
#include <atomic>

// Global termination flag
volatile std::sig_atomic_t g_running = 1;
// Global light sensor state (true = Dark, false = Light)
std::atomic<bool> g_lightState{false};

void signalHandler(int signum) {
    g_running = 0;
}

int main() {
    std::signal(SIGINT, signalHandler);

    // Create the MotorController object (controls motor via GPIO17 and GPIO27)
    MotorController motor;

    // Create the GPIOButton object on GPIO5.
    // Short press toggles FORWARD; long press toggles BACKWARD.
    GPIOButton button5(5,
        // Short press callback for forward operation
        [&motor]() {
            MotorState state = motor.get_state();
            if (state == MotorState::STOP) {
                motor.set_state(MotorState::FORWARD);
                std::cout << "Motor set to FORWARD" << std::endl;
            } else if (state == MotorState::FORWARD) {
                motor.set_state(MotorState::STOP);
                std::cout << "Motor FORWARD stopped" << std::endl;
            } else if (state == MotorState::BACKWARD) {
                std::cout << "Cannot set FORWARD: Motor is in BACKWARD state. Stop it first." << std::endl;
            }
        },
        // Long press callback for backward operation
        [&motor]() {
            MotorState state = motor.get_state();
            if (state == MotorState::STOP) {
                motor.set_state(MotorState::BACKWARD);
                std::cout << "Motor set to BACKWARD" << std::endl;
            } else if (state == MotorState::BACKWARD) {
                motor.set_state(MotorState::STOP);
                std::cout << "Motor BACKWARD stopped" << std::endl;
            } else if (state == MotorState::FORWARD) {
                std::cout << "Cannot set BACKWARD: Motor is in FORWARD state. Stop it first." << std::endl;
            }
        }
    );

    if (!button5.start()) {
        std::cerr << "Failed to start button event polling on GPIO5" << std::endl;
        return 1;
    }

    // Create the LightSensor object on GPIO16 (adjust as needed)
    LightSensor lightSensor(16, [](bool state) {
        g_lightState.store(state, std::memory_order_release);
    });
    lightSensor.start();

    std::cout << "Motor control via button on GPIO5 and light sensor monitoring started." << std::endl;
    std::cout << "Press Ctrl+C to exit." << std::endl;

    // Print initial sensor state
    auto last_print = std::chrono::steady_clock::now();
    bool last_state = g_lightState.load(std::memory_order_acquire);
    std::cout << "Light sensor state: " << (last_state ? "Dark" : "Light") << std::endl;

    // Main loop: check every 100ms; print state if it changes or 10 seconds have passed
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bool current_state = g_lightState.load(std::memory_order_acquire);
        auto now = std::chrono::steady_clock::now();
        if (current_state != last_state ||
            std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count() >= 10) {
            std::cout << "Light sensor state: " << (current_state ? "Dark" : "Light") << std::endl;
            last_print = now;
            last_state = current_state;
        }
    }

    button5.stop();
    lightSensor.stop();
    std::cout << "Exiting program." << std::endl;
    return 0;
}
