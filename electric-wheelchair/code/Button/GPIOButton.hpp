#ifndef GPIOBUTTON_HPP
#define GPIOBUTTON_HPP

#include <gpiod.hpp>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>

class GPIOButton {
public:
    GPIOButton(unsigned gpio,
               const std::function<void()>& shortCallback,
               const std::function<void()>& longCallback);
    ~GPIOButton();

    // Start polling for button events
    bool start();
    // Stop polling for button events
    void stop();

private:
    unsigned gpio_;  //The GPIO number used for the button
    std::function<void()> shortCallback_;  // Callback function triggered on a short button press
    std::function<void()> longCallback_;  // Callback function triggered on a long button press
    std::atomic<bool> running_;
    std::thread pollThread_;
    gpiod::chip chip_;
    gpiod::line line_;
    std::chrono::milliseconds debounceDelay_;  // Delay to prevent false triggers due to button bounce
    std::chrono::milliseconds longPressThreshold_;  // Time threshold to distinguish between short and long presses

    // Thread function that polls for button events and measures press duration
    void pollButton();
};

#endif // GPIOBUTTON_HPP
