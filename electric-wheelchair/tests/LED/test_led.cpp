#include "LEDController.hpp"
#include <iostream>
#include <chrono>
#include <thread>
int main() {
    try {
        // Blink exactly 3 times on GPIO9 and GPIO11
        LEDController led1(9);
        LEDController led2(11);
        const int BLINKS = 3;
        const auto ON  = std::chrono::milliseconds(200);
        const auto OFF = std::chrono::milliseconds(200);
        std::cout << "Blinking LEDs 3 times on GPIO9 and GPIO11\n";
        for (int i = 0; i < BLINKS; ++i) {
            led1.turnOn();
            led2.turnOn();
            std::this_thread::sleep_for(ON);
            led1.turnOff();
            led2.turnOff();
            std::this_thread::sleep_for(OFF);
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "LED test failed: " << e.what() << std::endl;
        return 1;
    }
}
