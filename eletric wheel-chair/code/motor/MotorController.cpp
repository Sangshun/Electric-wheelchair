#include "MotorController.hpp"
#include <gpiod.hpp>
#include <iostream>
#include <mutex>

// Define the GPIO chip name
constexpr char GPIO_CHIP[] = "gpiochip0";

// Parameterized constructor: allows setting the two output pins
MotorController::MotorController(unsigned in1_pin, unsigned in2_pin)
    : m_current_state(MotorState::STOP), m_emergency_flag(false)
{
    try {
        m_chip = gpiod::chip(GPIO_CHIP);
        m_in1 = m_chip.get_line(in1_pin);
        m_in2 = m_chip.get_line(in2_pin);

        gpiod::line_request config = {
            "motor_ctrl",
            gpiod::line_request::DIRECTION_OUTPUT,
            0
        };

        m_in1.request(config, 0);
        m_in2.request(config, 0);
    } catch (const std::exception& e) {
        std::cerr << "GPIO initialization failed: " << e.what() << std::endl;
        throw;
    }
}

// Default constructor uses pins 17 and 27
MotorController::MotorController() : MotorController(17, 27) {}

void MotorController::set_state(MotorState new_state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_emergency_flag.load())
        return;
    m_current_state = new_state;
    update_gpio();
}

void MotorController::emergency_stop() {
    m_emergency_flag.store(true);
    set_state(MotorState::STOP);
}

MotorState MotorController::get_state() const {
    return m_current_state.load();
}

void MotorController::update_gpio() {
    switch(m_current_state.load()) {
        case MotorState::FORWARD:
            m_in1.set_value(1);
            m_in2.set_value(0);
            break;
        case MotorState::BACKWARD:
            m_in1.set_value(0);
            m_in2.set_value(1);
            break;
        case MotorState::STOP:
        default:
            m_in1.set_value(0);
            m_in2.set_value(0);
            break;
    }
}

MotorController::~MotorController() {
    m_in1.release();
    m_in2.release();
}
