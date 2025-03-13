#pragma once
#include <functional>
#include <gpiod.hpp>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <system_error>
#include <stdexcept>

class PIRController {
public:
    using MotionCallback = std::function<void(bool detected)>;


    explicit PIRController(
        const std::string& chip_name = "gpiochip0",
        unsigned int pin = 25,
        MotionCallback cb = nullptr
    );


    PIRController(const PIRController&) = delete;
    PIRController& operator=(const PIRController&) = delete;


    ~PIRController();


    void start(unsigned int debounce_ms = 100);


    void stop() noexcept;


    void set_callback(MotionCallback new_cb);

private:

    void event_monitor();


    gpiod::chip m_chip;
    gpiod::line m_line;
    const unsigned int m_pin;


    std::thread m_monitor_thread;
    std::atomic_bool m_running{false};
    std::chrono::milliseconds m_debounce_time;


    MotionCallback m_callback;
    mutable std::mutex m_callback_mutex;
    std::atomic<bool> m_last_state{false};
};
