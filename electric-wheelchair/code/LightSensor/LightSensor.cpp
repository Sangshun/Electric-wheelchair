#include "LightSensor.hpp"
#include <cstdlib>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <cstring>

// Define the static exit_flag_ variable
std::atomic<bool> LightSensor::exit_flag_{false};

LightSensor::LightSensor(int gpio_pin, Callback callback)
    : gpio_pin_(gpio_pin), callback_(std::move(callback)), running_(false),
      chip_(nullptr), line_(nullptr)
{
    // Remove the following line to avoid installing a separate SIGINT handler:
    // std::signal(SIGINT, LightSensor::signal_handler);
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
    if (gpiod_line_request(line_, &config, 0) < 0) {
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
    
    while (running_) {
        // Instead of printing the state here, just call the callback
        bool state = gpiod_line_get_value(line_);
        // Call the callback only if you want to update the global variable; remove printing:
        callback_(state);
        
        // Sleep in short intervals so the thread checks the exit flag frequently
        const int intervals = 5;
        for (int i = 0; i < intervals && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Use a short timeout to wait for events
        struct timespec timeout = {0, 100 * 1000000};  // 100ms
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
