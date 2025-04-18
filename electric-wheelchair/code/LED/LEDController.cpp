#include "LEDController.hpp"
#include <iostream>

constexpr char GPIO_CHIP[] = "gpiochip0";

LEDController::LEDController(unsigned pin)
    : m_pin(pin), m_state(false)
{
    try {
        m_chip = gpiod::chip(GPIO_CHIP);
        m_line = m_chip.get_line(m_pin);
        gpiod::line_request config = {
            "led_ctrl",
            gpiod::line_request::DIRECTION_OUTPUT,
            0
        };
        m_line.request(config, 0);
    } catch (const std::exception& e) {
        std::cerr << "LEDController initialization failed: " << e.what() << std::endl;
        throw;
    }
}

LEDController::~LEDController() {
    m_line.release();
}

void LEDController::turnOn() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = true;
    updateGPIO();
}

void LEDController::turnOff() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = false;
    updateGPIO();
}

void LEDController::toggle() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = !m_state;
    updateGPIO();
}

bool LEDController::getState() const {
    return m_state;
}

void LEDController::updateGPIO() {
    m_line.set_value(m_state ? 1 : 0);
}
