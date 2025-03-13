#include "LightSensor.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <gpiod.h>

LightSensor::LightSensor(int gpio_pin, Callback callback)
    : gpio_pin_(gpio_pin), callback_(std::move(callback)) {
    signal(SIGINT, LightSensor::signal_handler);
    initialize_gpio();
}

LightSensor::~LightSensor() {
    stop();
    if (line_) gpiod_line_release(line_);
    if (chip_) gpiod_chip_close(chip_);
}

void LightSensor::signal_handler(int signal) {
    if (signal == SIGINT) {
        exit_flag_.store(true, std::memory_order_release);
        std::cout << "\nExit signal received" << std::endl;
    }
}

void LightSensor::initialize_gpio() {
    chip_ = gpiod_chip_open_by_name("gpiochip0");
    if (!chip_) {
        throw std::runtime_error("Failed to open GPIO chip: gpiochip0");
    }

    line_ = gpiod_chip_get_line(chip_, gpio_pin_);
    if (!line_) {
        gpiod_chip_close(chip_);
        throw std::runtime_error("Failed to get GPIO line " + std::to_string(gpio_pin_));
    }

    struct gpiod_line_request_config config = {};
    config.consumer = "light-sensor";
    config.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
    config.flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;  

    if (gpiod_line_request(line_, &config,0) < 0) {
        gpiod_line_release(line_);
        gpiod_chip_close(chip_);
        throw std::runtime_error("Failed to configure GPIO events on pin " + std::to_string(gpio_pin_));
    }
}

void LightSensor::start() {
    if (running_) return;
    running_ = true;
    event_thread_ = std::thread(&LightSensor::event_monitor, this);
}

void LightSensor::stop() {
    running_ = false;
    if (event_thread_.joinable()) {
        event_thread_.join();
    }
}

void LightSensor::event_monitor() {
    struct gpiod_line_event event;
    
    while (running_ && !exit_flag_) {
        
        bool state = gpiod_line_get_value(line_);
        std::cout << "Light status: " << (state ? "Dark" : "Light") << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); 

        
        struct timespec timeout = {0, 500 * 1000000};  
        int ret = gpiod_line_event_wait(line_, &timeout);
        if (ret > 0) {
            if (gpiod_line_event_read(line_, &event) == 0) {
                state = gpiod_line_get_value(line_);
                callback_(state); 
            }
        } else if (ret < 0) {
            std::cerr << "Event wait error" << std::endl;
            break;
        }
    }
}
