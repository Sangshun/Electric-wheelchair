#include "MotorController.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    // Create motor controller for motor1 (default: GPIO17 and GPIO27)
    MotorController motor1;
    // Create motor controller for motor2 (using GPIO22 and GPIO26)
    MotorController motor2(22, 26);

    std::cout << "Motor Test Program (Both Motors)" << std::endl;
    std::cout << "Motor1 (GPIO17 & GPIO27):" << std::endl;
    std::cout << "  f : Set to FORWARD" << std::endl;
    std::cout << "  b : Set to BACKWARD" << std::endl;
    std::cout << "Motor2 (GPIO22 & GPIO26):" << std::endl;
    std::cout << "  r : Set to RISE (FORWARD)" << std::endl;
    std::cout << "  l : Set to FALL (BACKWARD)" << std::endl;
    std::cout << "General:" << std::endl;
    std::cout << "  s : STOP both motors" << std::endl;
    std::cout << "  q : QUIT" << std::endl;

    char command;
    while (std::cin >> command && command != 'q') {
        if (command == 'f') {
            motor1.set_state(MotorState::FORWARD);
            std::cout << "Motor1 set to FORWARD" << std::endl;
        } else if (command == 'b') {
            motor1.set_state(MotorState::BACKWARD);
            std::cout << "Motor1 set to BACKWARD" << std::endl;
        } else if (command == 'r') {
            motor2.set_state(MotorState::FORWARD);
            std::cout << "Motor2 set to RISE" << std::endl;
        } else if (command == 'l') {
            motor2.set_state(MotorState::BACKWARD);
            std::cout << "Motor2 set to FALL" << std::endl;
        } else if (command == 's') {
            motor1.set_state(MotorState::STOP);
            motor2.set_state(MotorState::STOP);
            std::cout << "Both motors stopped" << std::endl;
        } else {
            std::cout << "Unknown command" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
