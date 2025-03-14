#include "UltrasonicSensor.hpp"
#include <chrono>
#include <thread>
#include <stdexcept>
#include <csignal>
#include <iostream>
#include <ctime>

std::atomic<bool> UltrasonicSensor::running{true};

UltrasonicSensor::UltrasonicSensor(int trig_pin, int echo_pin)
    : trig_pin_(trig_pin), echo_pin_(echo_pin) {
    initialize_gpio();
}

UltrasonicSensor::~UltrasonicSensor() {
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

    err = gpiod_line_request_output(trig_line, "trig", 0);
    if (err < 0) {
        perror("Failed to configure Trig");
        gpiod_chip_close(chip);
        return EXIT_FAILURE;
    }

    err = gpiod_line_request_input(echo_line, "echo");
    if (err < 0) {
        perror("Failed to configure Echo");
        gpiod_chip_close(chip);
        return EXIT_FAILURE;
    }

    while (keep_running) {  // Changed to controlled loop
        gpiod_line_set_value(trig_line, 1);
        usleep(10);
        gpiod_line_set_value(trig_line, 0);

        long start_time, end_time;
        int timeout = 0;

        long timeout_start = getMicrotime();
        while (gpiod_line_get_value(echo_line) == 0) {
            if (getMicrotime() - timeout_start > TIMEOUT_US) {
                timeout = 1;
                break;
            }
            if (!keep_running) break;  // Allow early exit
        }
        if (timeout || !keep_running) continue;
        start_time = getMicrotime();

        timeout_start = getMicrotime();
        while (gpiod_line_get_value(echo_line) == 1) {
            if (getMicrotime() - timeout_start > TIMEOUT_US) {
                timeout = 1;
                break;
            }
            if (!keep_running) break;  // Allow early exit
        }
        if (timeout || !keep_running) continue;
        end_time = getMicrotime();

        long duration = end_time - start_time;
        float distance = (duration * 0.0343) / 2;

        if (distance <= MAX_DISTANCE_CM) {
            printf("Distance: %.2f cm\n", distance);
        } else {
            printf("Out of range\n");
        }

        usleep(200000);
    }

    // Cleanup resources properly
    printf("\nCleaning up GPIO resources...\n");
    gpiod_line_set_value(trig_line, 0);
    gpiod_line_release(trig_line);
    gpiod_line_release(echo_line);
    gpiod_chip_close(chip);
    
    printf("Program terminated safely.\n");
    return EXIT_SUCCESS;
}
