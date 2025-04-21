#include "UltrasonicSensor.hpp"
#include <iostream>

int main() {
    // Use the same GPIO pins as your real setup
    UltrasonicSensor sensor(23, 24);
    // Perform a single distance measurement
    float distance = sensor.get_distance();
    if (distance <= 0.0f) {
        std::cerr << "ERROR: Non?positive distance measured: " 
                  << distance << " cm\n";
        return 1;  // Test fails
    }
    std::cout << "Measured distance: " 
              << distance << " cm\n";
    return 0;  // Test passes
}
