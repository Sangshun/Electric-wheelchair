#ifndef LIGHTSENSOR_HPP
#define LIGHTSENSOR_HPP

#include <functional>
#include <thread>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <string>
#include <gpiod.h>
#include <csignal>

/**
 * @brief LightSensor class encapsulates light sensor monitoring using libgpiod (C API).
 *
 * It monitors a specified GPIO pin for edge events and periodically reads the pin value.
 * The sensor state (true for Dark, false for Light) is delivered via a callback.
 */
class LightSensor {
public:
    using Callback = std::function<void(bool)>;

    LightSensor(int gpio_pin, Callback callback);
    ~LightSensor();

    // Start monitoring the light sensor
    void start();
    // Stop monitoring
    void stop();

private:
    int gpio_pin_;
    Callback callback_;
    std::atomic<bool> running_;
    // Make exit_flag_ static so it can be used in the static signal handler if needed.
    static std::atomic<bool> exit_flag_;
    std::thread event_thread_;

    struct gpiod_chip* chip_;
    struct gpiod_line* line_;

    // Initialize GPIO for the sensor
    void initialize_gpio();
    // Thread function to monitor events
    void event_monitor();
    // (Removed the SIGINT handler installation)
    static void signal_handler(int signal);
};

#endif // LIGHTSENSOR_HPP
