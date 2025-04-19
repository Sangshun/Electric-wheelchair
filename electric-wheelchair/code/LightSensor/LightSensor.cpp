#include "LightSensor.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <gpiod.h>
#include <iostream>    // for std::cout, std::cerr
#include <ostream>     // for std::endl
#include <stdexcept>   // for std::runtime_error
#include <string>      // for std::to_string

// Constructor, initialize gpio_pin_ and callback_, and set the signal processing function
LightSensor::LightSensor(int gpio_pin, Callback callback)
    : gpio_pin_(gpio_pin), callback_(std::move(callback)) {
    // Set the signal processing function. When the SIGINT signal is received, call the LightSensor::signal_handler function
    signal(SIGINT, LightSensor::signal_handler);
    // Initialize GPIO
    initialize_gpio();
}

// Destructor, used to release the resources occupied by the LightSensor object
LightSensor::~LightSensor() {
    // Stop sensor
    stop();
    // Release GPIO lines
    if (line_) gpiod_line_release(line_);
    // Turn off the GPIO chip
    if (chip_) gpiod_chip_close(chip_);
}

// Define a signal processing function
void LightSensor::signal_handler(int signal) {
    // If a SIGINT signal is received
    if (signal == SIGINT) {
        // Set the exit flag to true
        exit_flag_.store(true, std::memory_order_release);
        // Output the information received by the exit signal
        std::cout << "\nExit signal received" << std::endl;
    }
}

void LightSensor::initialize_gpio() {
    // Turn on the GPIO chip
    chip_ = gpiod_chip_open_by_name("gpiochip0");
    if (!chip_) {
        // If the opening fails, an exception is thrown
        throw std::runtime_error("Failed to open GPIO chip: gpiochip0");
    }

    // Get GPIO pins
    line_ = gpiod_chip_get_line(chip_, gpio_pin_);
    if (!line_) {
        // If the acquisition fails, turn off the GPIO chip and throw an exception
        gpiod_chip_close(chip_);
        throw std::runtime_error("Failed to get GPIO line " + std::to_string(gpio_pin_));
    }

    // Configuring GPIO Pins
    struct gpiod_line_request_config config = {};
    config.consumer = "light-sensor";
    config.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
    config.flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;  

    // Request GPIO pin
    if (gpiod_line_request(line_, &config,0) < 0) {
        // If the request fails, release the GPIO pin, turn off the GPIO chip, and throw an exception
        gpiod_line_release(line_);
        gpiod_chip_close(chip_);
        throw std::runtime_error("Failed to configure GPIO events on pin " + std::to_string(gpio_pin_));
    }
}

// Start Sensor
void LightSensor::start() {
    // If the sensor is already running, it returns
    if (running_) return;
    // Set the sensor to operating state
    running_ = true;
    // Create a thread to monitor sensor events
    event_thread_ = std::thread(&LightSensor::event_monitor, this);
}

// Stop light sensor
void LightSensor::stop() {
    // Set the run flag to false
    running_ = false;
    // If the event thread is joinable
    if (event_thread_.joinable()) {
        // Connection event thread
        event_thread_.join();
    }
}

void LightSensor::event_monitor() {
    // Define a gpiod_line_event structure variable
    struct gpiod_line_event event;
    
    // When running_ is true and exit_flag_ is false, the loop executes
    while (running_ && !exit_flag_) {
        
        // Get the current value of the GPIO port
        bool state = gpiod_line_get_value(line_);
        // Output current light status
        std::cout << "Light status: " << (state ? "Dark" : "Light") << std::endl;
        // Sleep for 500 milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); 

        
        // Define a timespec structure variable and set the timeout to 500 milliseconds
        struct timespec timeout = {0, 500 * 1000000};  
        // Wait for the GPIO port event to occur
        int ret = gpiod_line_event_wait(line_, &timeout);
        if (ret > 0) {
            // Reading events
            if (gpiod_line_event_read(line_, &event) == 0) {
                // Get the current value of the GPIO port
                state = gpiod_line_get_value(line_);
                // Calling callback function
                callback_(state); 
            }
        // If the wait event fails
        } else if (ret < 0) {
            // Output error information
            std::cerr << "Event wait error" << std::endl;
            // Exit the loopExit the loop
            break;
        }
    }
}
