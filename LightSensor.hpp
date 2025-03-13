#ifndef LIGHT_SENSOR_HPP
#define LIGHT_SENSOR_HPP

#include <atomic>
#include <functional>
#include <gpiod.h>
#include <csignal>
#include <thread> 

class LightSensor {
public:
    using Callback = std::function<void(bool)>;

    LightSensor(int gpio_pin, Callback callback);
    ~LightSensor();

    void start();
    void stop();
    static void signal_handler(int signal);
    static bool get_exit_flag() { return exit_flag_.load(); } 
    static inline std::atomic<bool> exit_flag_{false};

private:
    const int gpio_pin_;
    Callback callback_;
    std::atomic<bool> running_{false};
    std::thread event_thread_;

    gpiod_chip* chip_ = nullptr;
    gpiod_line* line_ = nullptr;
    //static inline std::atomic<bool> exit_flag_{false};

    void initialize_gpio();
    void event_monitor();
};

#endif // LIGHT_SENSOR_HPP
