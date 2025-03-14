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
    float distance = (duration.count() * 0.0343) / 2;
    std::cerr << "[Debug] Duration: " << duration.count() << "us, Distance: " << distance << "cm\n";
    return distance;
}

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
