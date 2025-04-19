#ifndef ULTRASONIC_SENSOR_HPP
#define ULTRASONIC_SENSOR_HPP

#include <atomic>
#include <mutex>
#include <gpiod.h>
#include <string>

class UltrasonicSensor {
public:
    UltrasonicSensor(int trig_pin, int echo_pin);
    ~UltrasonicSensor();
    
    float get_distance();
    static void signal_handler(int signal);
    static std::atomic<bool> running;
    std::string get_distance_str();


private:
    const int trig_pin_;
    const int echo_pin_;
    gpiod_chip* chip_ = nullptr;
    gpiod_line* trig_line_ = nullptr;
    gpiod_line* echo_line_ = nullptr;
    std::mutex gpio_mutex_;

    void initialize_gpio();
    void trigger_pulse() const;
    float measure_pulse();
    void cleanup_gpio();
    void reset_gpio();
};

#endif 
