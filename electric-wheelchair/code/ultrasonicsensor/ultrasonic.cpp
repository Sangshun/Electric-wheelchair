#include "UltrasonicSensor.hpp"
#include <chrono>
#include <thread>
#include <stdexcept>
#include <csignal>
#include <iostream>
#include <ctime>
#include <sstream>

std::atomic<bool> UltrasonicSensor::running{true};

UltrasonicSensor::UltrasonicSensor(int trig_pin, int echo_pin)
    : trig_pin_(trig_pin), echo_pin_(echo_pin) {
    initialize_gpio();
}

UltrasonicSensor::~UltrasonicSensor() {
    stop(); // Ensure the sensor thread is stopped before cleanup
    cleanup_gpio();
}

void UltrasonicSensor::signal_handler(int signal) {
    (void)signal;
    running.store(false, std::memory_order_release);
}

void UltrasonicSensor::reset_gpio() {
    gpiod_line_release(echo_line_);
    echo_line_ = gpiod_chip_get_line(chip_, echo_pin_);
    if (gpiod_line_request_both_edges_events(echo_line_, "echo") < 0) {
        throw std::runtime_error("Failed to reconfigure Echo");
    }
}

void UltrasonicSensor::initialize_gpio() {
    chip_ = gpiod_chip_open_by_name("gpiochip0");
    if (!chip_) throw std::runtime_error("Failed to open GPIO chip");

    trig_line_ = gpiod_chip_get_line(chip_, trig_pin_);
    echo_line_ = gpiod_chip_get_line(chip_, echo_pin_);
    if (!trig_line_ || !echo_line_) {
        cleanup_gpio();
        throw std::runtime_error("Failed to get GPIO lines");
    }

    if (gpiod_line_request_output(trig_line_, "trig", 0) < 0) {
        cleanup_gpio();
        throw std::runtime_error("Failed to configure Trig");
    }
   
    if (gpiod_line_request_both_edges_events(echo_line_, "echo") < 0) {
        cleanup_gpio();
        throw std::runtime_error("Failed to configure Echo");
    }
}

void UltrasonicSensor::trigger_pulse() const {
    // Set trig low, wait, then high for 10 microseconds, then low
    gpiod_line_set_value(trig_line_, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2)); 
    gpiod_line_set_value(trig_line_, 1);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    gpiod_line_set_value(trig_line_, 0);
}

float UltrasonicSensor::measure_pulse() {
    struct gpiod_line_event event;
    constexpr int timeout_us = 38000; 
    struct timespec ts = {
        .tv_sec = timeout_us / 1000000,
        .tv_nsec = (timeout_us % 1000000) * 1000
    };

     auto wait_event = [&](int type) -> bool {
        int ret = gpiod_line_event_wait(echo_line_, &ts);
        if (ret <= 0) {
            std::cerr << "Event wait failed (ret=" << ret << ")\n";
            return false;
        }
        ret = gpiod_line_event_read(echo_line_, &event);
        if (ret != 0) {
            std::cerr << "Event read failed (ret=" << ret << ")\n";
            return false;
        }
        return event.event_type == type;
    };
    
	if (!wait_event(GPIOD_LINE_EVENT_RISING_EDGE)) {
    std::cerr << "[Debug] Rising edge not detected, resetting GPIO\n";
    reset_gpio();
    return -1;
	}

    auto start = std::chrono::steady_clock::now();

    if (!wait_event(GPIOD_LINE_EVENT_FALLING_EDGE)) {
        std::cerr << "[Debug] Falling edge not detected\n";
        return -1;
    }
    auto end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    float distance = (duration.count() * 0.0343f) / 2.0f;
    return distance;
}

void UltrasonicSensor::cleanup_gpio() {
    if (trig_line_) {
        gpiod_line_set_value(trig_line_, 0);
        gpiod_line_release(trig_line_);
    }
    if (echo_line_) gpiod_line_release(echo_line_);
    if (chip_) gpiod_chip_close(chip_);
}

float UltrasonicSensor::perform_measurement() {
    std::lock_guard<std::mutex> lock(gpio_mutex_);
    trigger_pulse();
    float distance = measure_pulse();
    if (distance < 2 || distance > 450) { 
        return -1;
    }
    return distance;
}

void UltrasonicSensor::measurement_loop() {
    while (sensor_thread_running_.load()) {
        float distance = perform_measurement();
        {
            std::lock_guard<std::mutex> lock(distance_mutex_);
            latest_distance_ = distance;
        }
        if (measurement_callback_) {
            measurement_callback_(distance);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void UltrasonicSensor::start() {
    if (sensor_thread_running_.load()) {
        return; // Already running
    }
    sensor_thread_running_.store(true);
    sensor_thread_ = std::thread(&UltrasonicSensor::measurement_loop, this);
}

void UltrasonicSensor::stop() {
    sensor_thread_running_.store(false);
    if (sensor_thread_.joinable()) {
        sensor_thread_.join();
    }
}

float UltrasonicSensor::get_latest_distance() {
    std::lock_guard<std::mutex> lock(distance_mutex_);
    return latest_distance_;
}

std::string UltrasonicSensor::get_distance_str() {
    float distance = get_latest_distance();
    if (distance < 0 || distance > 150)
        return "Out of range";
    else {
        std::ostringstream oss;
        oss << distance << " cm";
        return oss.str();
    }
}

void UltrasonicSensor::set_callback(std::function<void(float)> cb) {
    measurement_callback_ = cb;
}

