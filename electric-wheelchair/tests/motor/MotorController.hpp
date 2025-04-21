#ifndef MOTORCONTROLLER_HPP
#define MOTORCONTROLLER_HPP

#include <gpiod.hpp>
#include <mutex>
#include <atomic>
#include <iostream>

// Enumeration for motor states
enum class MotorState { STOP, FORWARD, BACKWARD };

/**
 * @brief MotorController class encapsulates motor control using GPIO pins.
 *
 * This class controls a motor via two GPIO pins (GPIO17 and GPIO27). It supports
 * running the motor forward, backward, or stopping it.
 */
class MotorController {
public:
    MotorController();
    ~MotorController();

    // Set the motor state (FORWARD, BACKWARD, or STOP)
    void set_state(MotorState new_state);

    // Immediately stop the motor and set the emergency flag
    void emergency_stop();

    // Get the current motor state
    MotorState get_state() const;

private:
    // Update the GPIO outputs based on the current state
    void update_gpio();

    // GPIO resources
    gpiod::chip m_chip;
    gpiod::line m_in1;
    gpiod::line m_in2;
    
    mutable std::mutex m_mutex;
    std::atomic<MotorState> m_current_state;
    std::atomic_bool m_emergency_flag;
};

#endif // MOTORCONTROLLER_HPP
