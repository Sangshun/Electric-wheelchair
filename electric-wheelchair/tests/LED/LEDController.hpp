#ifndef LEDCONTROLLER_HPP
#define LEDCONTROLLER_HPP

#include <gpiod.hpp>
#include <mutex>

class LEDController {
public:
    // Constructor: initialize LED on a given GPIO pin
    LEDController(unsigned pin);
    
    // Destructor
    ~LEDController();
    
    // Turn the LED on
    void turnOn();
    
    // Turn the LED off
    void turnOff();
    
    // Toggle the LED state
    void toggle();
    
    // Get current LED state (true = on, false = off)
    bool getState() const;
    
private:
    // Update the GPIO line based on current state
    void updateGPIO();
    
    gpiod::chip m_chip;
    gpiod::line m_line;
    unsigned m_pin;
    bool m_state;
    std::mutex m_mutex;
};

#endif // LEDCONTROLLER_HPP
