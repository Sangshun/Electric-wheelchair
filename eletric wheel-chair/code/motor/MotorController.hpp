#ifndef MOTORCONTROLLER_HPP
#define MOTORCONTROLLER_HPP

#include <atomic>
#include <mutex>
#include <gpiod.hpp> // Include the libgpiod header for GPIO control

enum class MotorState {
    STOP,
    FORWARD,
    BACKWARD
};

class MotorController {
public:
    // Overloaded constructor to specify custom GPIO pins
    MotorController(unsigned in1_pin, unsigned in2_pin);
    
    // Default constructor uses pins 17 and 27
    MotorController();

    void set_state(MotorState new_state);
    MotorState get_state() const;
    void emergency_stop();

    ~MotorController();

private:
    void update_gpio();

    gpiod::chip m_chip;
    gpiod::line m_in1;
    gpiod::line m_in2;
    
    std::atomic<MotorState> m_current_state;
    std::atomic<bool> m_emergency_flag;
    std::mutex m_mutex;
};

#endif // MOTORCONTROLLER_HPP
