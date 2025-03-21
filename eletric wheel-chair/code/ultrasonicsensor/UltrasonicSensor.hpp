#ifndef ULTRASONIC_SENSOR_HPP
#define ULTRASONIC_SENSOR_HPP

#include <atomic>
#include <mutex>
#include <gpiod.h>
#include <string>
#include <thread>
#include <functional>

class UltrasonicSensor {
public:
    // Constructor and destructor
    UltrasonicSensor(int trig_pin, int echo_pin);
    ~UltrasonicSensor();
    
    // Get the latest distance measurement (non-blocking)
    float get_latest_distance();

    // Get the latest distance measurement as a string
    std::string get_distance_str();

    // Start the sensor measurement thread
    void start();

    // Stop the sensor measurement thread
    void stop();

    // Set a callback function to be notified when a new measurement is available
    void set_callback(std::function<void(float)> cb);

    // Static signal handler for graceful termination
    static void signal_handler(int signal);
    static std::atomic<bool> running;

private:
    const int trig_pin_;
    const int echo_pin_;
    gpiod_chip* chip_ = nullptr;
    gpiod_line* trig_line_ = nullptr;
    gpiod_line* echo_line_ = nullptr;
    std::mutex gpio_mutex_;
    std::mutex distance_mutex_;
    float latest_distance_ = -1.0f;
    std::thread sensor_thread_;
    std::atomic<bool> sensor_thread_running_{false};

    // Optional callback function to notify new measurements
    std::function<void(float)> measurement_callback_;

    // Initialize GPIO resources
    void initialize_gpio();

    // Trigger the ultrasonic pulse
    void trigger_pulse() const;

    // Measure the pulse duration and calculate distance (blocking call)
    float measure_pulse();

    // Clean up GPIO resources
    void cleanup_gpio();

    // Reset GPIO in case of error
    void reset_gpio();

    // Perform a single distance measurement (blocking call)
    float perform_measurement();

    // Sensor measurement loop running in a separate thread
    void measurement_loop();
};

#endif

