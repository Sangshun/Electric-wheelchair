#include "MotorController.hpp"
#include <iostream>
#include <thread>
#include <chrono>

// The program simulates button presses via keyboard input.
// 'f' sets the motor to FORWARD.
// 'b' sets the motor to BACKWARD.
// 's' stops the motor.
// 'q' quits the program.

int main() {
    // Create the motor controller object.
    MotorController motor;

    std::cout << "Motor Control Test Program:" << std::endl;
    std::cout << "Press 'f' for FORWARD, 'b' for BACKWARD, 's' for STOP, 'q' to quit." << std::endl;

    bool running = true;
    char input;
    while (running) {
        std::cin >> input;
        switch (input) {
            case 'f': case 'F':
                motor.set_state(MotorState::FORWARD);
                std::cout << "Motor set to FORWARD" << std::endl;
                break;
            case 'b': case 'B':
                motor.set_state(MotorState::BACKWARD);
                std::cout << "Motor set to BACKWARD" << std::endl;
                break;
            case 's': case 'S':
                motor.set_state(MotorState::STOP);
                std::cout << "Motor stopped" << std::endl;
                break;
            case 'q': case 'Q':
                running = false;
                break;
            default:
                std::cout << "Invalid input. Use 'f', 'b', 's', or 'q'." << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Exiting test program..." << std::endl;
    return 0;
}
