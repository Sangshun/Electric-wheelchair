#ifndef ULTRASONIC_SENSOR_HPP
#define ULTRASONIC_SENSOR_HPP  // Header guard to prevent multiple inclusion

#include <atomic>       // For thread-safe flag operations
#include <mutex>        // For GPIO access synchronization
#include <gpiod.h>      // Linux GPIO character device interface
#include <string>       // For string-based distance representation

class UltrasonicSensor {
public:
    UltrasonicSensor(int trig_pin, int echo_pin);  // Constructor with GPIO pin assignments
    ~UltrasonicSensor();
    
    float get_distance();                           // Primary distance measurement interface
    static void signal_handler(int signal);         // OS signal handler for graceful shutdown
    static std::atomic<bool> running;               // Global control flag for threads
    std::string get_distance_str();                 // Formatted distance string output

private:
    const int trig_pin_;     // Trigger pin (BCM numbering)
    const int echo_pin_;     // Echo pin (BCM numbering)
    gpiod_chip* chip_ = nullptr;  // GPIO chip handle
    gpiod_line* trig_line_ = nullptr;  // Trigger line object
    gpiod_line* echo_line_ = nullptr;  // Echo line object
    std::mutex gpio_mutex_;  // Mutex for GPIO operation synchronization

    void initialize_gpio();    // GPIO hardware initialization
    void trigger_pulse() const;  // Generate ultrasonic trigger signal
    float measure_pulse();     // Calculate distance from pulse timing
    void cleanup_gpio();       // GPIO resource release
    void reset_gpio();         // Echo line recovery mechanism
};

#endif 
