#include "UltrasonicSensor.hpp"
#include <chrono>
#include <thread>
#include <stdexcept>
#include <csignal>
#include <iostream>
#include <ctime>
#include <sstream>

std::atomic<bool> UltrasonicSensor::running{true};  // Controls sensor operation state

UltrasonicSensor::UltrasonicSensor(int trig_pin, int echo_pin)
    : trig_pin_(trig_pin), echo_pin_(echo_pin) {
    initialize_gpio();  // Setup GPIO hardware interface
}

UltrasonicSensor::~UltrasonicSensor() {
    cleanup_gpio();  // Release hardware resources
}

void UltrasonicSensor::signal_handler(int signal) {
    (void)signal;
    running.store(false, std::memory_order_release);  // Update atomic flag
}

void UltrasonicSensor::reset_gpio() {
    gpiod_line_release(echo_line_);
    echo_line_ = gpiod_chip_get_line(chip_, echo_pin_);  // Re-acquire echo line
     if (gpiod_line_request_both_edges_events(echo_line_, "echo") < 0) {  // Reconfigure edge detection
        throw std::runtime_error("Failed to reconfigure Echo");
    }
}

void UltrasonicSensor::initialize_gpio() {
     chip_ = gpiod_chip_open_by_name("gpiochip0");  // Access system GPIO
    if (!chip_) throw std::runtime_error("Failed to open GPIO chip");

    trig_line_ = gpiod_chip_get_line(chip_, trig_pin_);  // Trigger control line
    echo_line_ = gpiod_chip_get_line(chip_, echo_pin_);  // Echo detection line
    if (!trig_line_ || !echo_line_) {  // Validate pin acquisition
        cleanup_gpio();
        throw std::runtime_error("Failed to get GPIO lines");
    }

     if (gpiod_line_request_output(trig_line_, "trig", 0) < 0) {  // Configure trigger as output
        cleanup_gpio();
        throw std::runtime_error("Failed to configure Trig");
    }

    
    if (gpiod_line_request_both_edges_events(echo_line_, "echo") < 0) {  // Enable echo edge detection
        cleanup_gpio();
        throw std::runtime_error("Failed to configure Echo");
    }
}

void UltrasonicSensor::trigger_pulse() const {
    gpiod_line_set_value(trig_line_, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));  // Stabilization delay
    gpiod_line_set_value(trig_line_, 1);  // Start pulse
    std::this_thread::sleep_for(std::chrono::microseconds(10));  // Maintain pulse width
    gpiod_line_set_value(trig_line_, 0);
}

float UltrasonicSensor::measure_pulse() {
    struct gpiod_line_event event;
    constexpr int timeout_us = 38000; 
    struct timespec ts = {  // Convert timeout to timespec
        .tv_sec = timeout_us / 1000000,
        .tv_nsec = (timeout_us % 1000000) * 1000
    };

    auto wait_event = [&](int type) -> bool {
        int ret = gpiod_line_event_wait(echo_line_, &ts);  // Wait for event
        if (ret <= 0) {
            std::cerr << "Event wait failed (ret=" << ret << ")\n";
            return false;
        }
        ret = gpiod_line_event_read(echo_line_, &event);  // Read occurred event
        if (ret != 0) {
            std::cerr << "Event read failed (ret=" << ret << ")\n";
            return false;
        }
         return event.event_type == type;  // Check event type match
    };
    
	if (!wait_event(GPIOD_LINE_EVENT_RISING_EDGE)) {  // Wait for echo start
    std::cerr << "[Debug] Rising edge not detected, resetting GPIO\n";
    reset_gpio();  // Recover from error state
    return -1;
    }

    auto start = std::chrono::steady_clock::now();  // Pulse start time

    if (!wait_event(GPIOD_LINE_EVENT_FALLING_EDGE)) {  // Wait for echo end
        std::cerr << "[Debug] Falling edge not detected\n";
        return -1;
    }
    auto end = std::chrono::steady_clock::now();  // Pulse end time

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    float distance = (duration.count() * 0.0343) / 2;  // Convert μs to cm (sound speed 343m/s)
    //std::cerr << "[Debug] Duration: " << duration.count() << "us, Distance: " << distance << "cm\n";
    return distance;
}

void UltrasonicSensor::cleanup_gpio() {
    if (trig_line_) {
        gpiod_line_set_value(trig_line_, 0);  // Ensure trigger off
        gpiod_line_release(trig_line_);
    }
    if (echo_line_) gpiod_line_release(echo_line_);
    if (chip_) gpiod_chip_close(chip_);
}

float UltrasonicSensor::get_distance() {
    std::lock_guard<std::mutex> lock(gpio_mutex_);  // Thread safety
    trigger_pulse();  // Initiate measurement


    float distance = measure_pulse();  // Get raw measurement

    if (distance < 2 || distance > 450) {  // Validate physical range 
        return -1;
    }

    return distance;
}

std::string UltrasonicSensor::get_distance_str() {
    float distance = get_distance();
  
    if (distance < 0 || distance > 150)  // Filter for display range
        return "Out of range";
    else {
        std::ostringstream oss;
        oss << distance << " cm";
        return oss.str();
    }
}
