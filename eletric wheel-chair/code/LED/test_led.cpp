#include "LEDController.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace std::chrono;

int main() {
    try {
        // Instantiate LED controllers for GPIO9 and GPIO11
        LEDController led1(9);
        LEDController led2(11);

        std::cout << "Blinking LEDs on GPIO9 and GPIO11" << std::endl;
        while (true) {
            led1.turnOn();
            led2.turnOn();
            std::this_thread::sleep_for(seconds(1));
            led1.turnOff();
            led2.turnOff();
            std::this_thread::sleep_for(seconds(1));
        }
    } catch (const std::exception &e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
